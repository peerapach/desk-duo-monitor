#!/usr/bin/env python3
"""Forward CodexBar usage_updated events to an ESP32 usage receiver."""

import argparse
import json
import math
import re
import sys
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime, timezone
from pathlib import Path


MAX_EVENT_BYTES = 4096
DEFAULT_TIMEOUT_SECONDS = 5.0
_TIMESTAMP_PATTERN = re.compile(
    r"\A\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d+)?(?:Z|[+-]\d{2}:\d{2})\Z"
)


class DeliveryError(RuntimeError):
    """A sanitized failure while sending usage to the receiver."""


def _reject_json_constant(_value):
    raise ValueError("invalid JSON")


def _parse_json(text):
    return json.loads(text, parse_constant=_reject_json_constant)


def _utc_timestamp(value, field_name):
    if not isinstance(value, str) or not _TIMESTAMP_PATTERN.fullmatch(value):
        raise ValueError(f"invalid {field_name}")
    try:
        parsed = datetime.fromisoformat(value[:-1] + "+00:00" if value.endswith("Z") else value)
    except ValueError:
        raise ValueError(f"invalid {field_name}") from None
    if parsed.tzinfo is None or parsed.utcoffset() is None:
        raise ValueError(f"invalid {field_name}")
    utc = parsed.astimezone(timezone.utc)
    timespec = "microseconds" if utc.microsecond else "seconds"
    return utc.isoformat(timespec=timespec).replace("+00:00", "Z")


def _finite_number(value, field_name):
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValueError(f"invalid {field_name}")
    try:
        finite = math.isfinite(value)
    except (OverflowError, TypeError):
        finite = False
    if not finite:
        raise ValueError(f"invalid {field_name}")
    return value


def _usage_fraction(value, field_name):
    fraction = _finite_number(value, field_name)
    if fraction < 0 or fraction > 1:
        raise ValueError(f"invalid {field_name}")
    percent = fraction * 100
    return int(percent) if percent.is_integer() else percent


def _build_window(event, *, fraction_field, minutes_field, reset_field):
    fraction_present = fraction_field in event
    if fraction_present:
        percent = _usage_fraction(event[fraction_field], fraction_field)
    else:
        percent = None

    minutes = None
    if minutes_field in event:
        minutes = _finite_number(event[minutes_field], minutes_field)
        if minutes <= 0:
            raise ValueError(f"invalid {minutes_field}")

    reset_at = None
    if reset_field in event:
        reset_at = _utc_timestamp(event[reset_field], reset_field)

    if not fraction_present:
        return None

    window = {"usedPercent": percent}
    if minutes is not None:
        window["windowMinutes"] = minutes
    if reset_at is not None:
        window["resetsAt"] = reset_at
    return window


def normalize_event(event):
    """Convert one CodexBar usage_updated event to the device payload schema."""
    if not isinstance(event, dict):
        raise ValueError("invalid event")
    if event.get("event") != "usage_updated":
        raise ValueError("invalid event")
    provider = event.get("provider")
    if provider not in ("claude", "codex"):
        raise ValueError("invalid provider")

    payload = {
        "schemaVersion": 1,
        "type": "usage",
        "provider": provider,
        "updatedAt": _utc_timestamp(event.get("timestamp"), "timestamp"),
    }

    primary = _build_window(
        event,
        fraction_field="usagePercent",
        minutes_field="windowMinutes",
        reset_field="resetAt",
    )
    secondary = _build_window(
        event,
        fraction_field="secondaryUsagePercent",
        minutes_field="secondaryWindowMinutes",
        reset_field="secondaryResetAt",
    )
    if primary is not None:
        payload["primary"] = primary
    if secondary is not None:
        payload["secondary"] = secondary
    return payload


def _validate_endpoint(endpoint):
    if not isinstance(endpoint, str) or not endpoint or endpoint != endpoint.strip():
        raise ValueError("invalid configuration")
    if any(character.isspace() or ord(character) < 0x20 or 0x7F <= ord(character) <= 0x9F
           for character in endpoint):
        raise ValueError("invalid configuration")
    if "\\" in endpoint or "?" in endpoint or "#" in endpoint:
        raise ValueError("invalid configuration")
    try:
        parsed = urllib.parse.urlsplit(endpoint)
        port = parsed.port
    except ValueError:
        raise ValueError("invalid configuration") from None
    if parsed.scheme not in ("http", "https") or not parsed.hostname:
        raise ValueError("invalid configuration")
    if parsed.username is not None or parsed.password is not None:
        raise ValueError("invalid configuration")
    if parsed.path != "/api/usage" or parsed.query or parsed.fragment:
        raise ValueError("invalid configuration")
    if port is not None and not 1 <= port <= 65535:
        raise ValueError("invalid configuration")


def _validated_config(config):
    if not isinstance(config, dict):
        raise ValueError("invalid configuration")
    endpoint = config.get("endpoint")
    token = config.get("bearerToken")
    timeout = config.get("timeoutSeconds", DEFAULT_TIMEOUT_SECONDS)
    _validate_endpoint(endpoint)
    if not isinstance(token, str) or not token or not token.isascii() or token.strip() != token:
        raise ValueError("invalid configuration")
    if any(character.isspace() or ord(character) < 0x20 or 0x7F <= ord(character) <= 0x9F
           for character in token):
        raise ValueError("invalid configuration")
    if isinstance(timeout, bool) or not isinstance(timeout, (int, float)):
        raise ValueError("invalid configuration")
    try:
        timeout_is_finite = math.isfinite(timeout)
    except (OverflowError, TypeError):
        timeout_is_finite = False
    if not timeout_is_finite or not 0.1 <= timeout <= 30:
        raise ValueError("invalid configuration")
    return {"endpoint": endpoint, "bearerToken": token, "timeoutSeconds": float(timeout)}


def load_config(path):
    """Load and validate bridge configuration without exposing its values."""
    try:
        config = _parse_json(Path(path).read_text(encoding="utf-8"))
        return _validated_config(config)
    except (OSError, UnicodeError, json.JSONDecodeError, ValueError):
        raise ValueError("invalid configuration") from None


class _RejectRedirects(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, request, response, code, message, headers, new_url):
        return None


def send_usage(payload, config):
    """POST one normalized payload; never follow a receiver redirect."""
    validated = _validated_config(config)
    if not isinstance(payload, dict):
        raise ValueError("invalid usage payload")
    try:
        body = json.dumps(payload, separators=(",", ":"), ensure_ascii=False, allow_nan=False).encode("utf-8")
    except (TypeError, ValueError, UnicodeError):
        raise ValueError("invalid usage payload") from None

    request = urllib.request.Request(
        validated["endpoint"],
        data=body,
        headers={
            "Content-Type": "application/json",
            "Authorization": f"Bearer {validated['bearerToken']}",
        },
        method="POST",
    )
    opener = urllib.request.build_opener(
        urllib.request.ProxyHandler({}),
        _RejectRedirects(),
    )
    try:
        with opener.open(request, timeout=validated["timeoutSeconds"]) as response:
            status = response.getcode()
    except urllib.error.HTTPError as error:
        error.close()
        raise DeliveryError("delivery failed") from None
    except Exception:
        raise DeliveryError("delivery failed") from None
    if not 200 <= status < 300:
        raise DeliveryError("delivery failed")


def _read_event_bytes():
    stream = getattr(sys.stdin, "buffer", None)
    if stream is not None:
        return stream.read(MAX_EVENT_BYTES + 1)
    return sys.stdin.read(MAX_EVENT_BYTES + 1).encode("utf-8")


def main(argv=None):
    parser = argparse.ArgumentParser(description="Forward CodexBar usage_updated events to an ESP32 receiver")
    parser.add_argument("--config", type=Path, help="private JSON bridge configuration file")
    parser.add_argument("--dry-run", action="store_true", help="normalize and print the payload without networking")
    args = parser.parse_args(argv)

    try:
        raw_event = _read_event_bytes()
        if len(raw_event) > MAX_EVENT_BYTES:
            raise ValueError("invalid event")
        event_text = raw_event.decode("utf-8")
        event = _parse_json(event_text)
        payload = normalize_event(event)
        if args.dry_run:
            print(json.dumps(payload, separators=(",", ":"), ensure_ascii=False, allow_nan=False))
            return 0
        if args.config is None:
            raise ValueError("invalid configuration")
        config = load_config(args.config)
        send_usage(payload, config)
        return 0
    except DeliveryError:
        print("codexbar sender: delivery failed", file=sys.stderr)
        return 1
    except (UnicodeError, ValueError, json.JSONDecodeError):
        print("codexbar sender: invalid input or configuration", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
