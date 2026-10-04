import contextlib
import functools
import io
import json
import math
import multiprocessing
import signal
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from unittest.mock import patch

try:
    from bridge import codexbar_sender as sender
except ModuleNotFoundError as error:
    if error.name != "bridge":
        raise
    sender = None


def load_config(*args, **kwargs):
    return sender.load_config(*args, **kwargs)


def main(*args, **kwargs):
    return sender.main(*args, **kwargs)


def normalize_event(*args, **kwargs):
    return sender.normalize_event(*args, **kwargs)


def send_usage(*args, **kwargs):
    return sender.send_usage(*args, **kwargs)


DeliveryError = getattr(sender, "DeliveryError", RuntimeError)


TOKEN = "local-test-device-token"
PAYLOAD = {
    "schemaVersion": 1,
    "type": "usage",
    "provider": "codex",
    "updatedAt": "2026-10-04T03:00:00Z",
    "primary": {"usedPercent": 0, "windowMinutes": 300, "resetsAt": "2026-10-04T08:00:00Z"},
}


def event(**overrides):
    value = {
        "event": "usage_updated",
        "provider": "codex",
        "timestamp": "2026-10-04T10:00:00+07:00",
        "usagePercent": 0,
        "windowMinutes": 300,
        "resetAt": "2026-10-04T15:00:00+07:00",
    }
    value.update(overrides)
    return value


def config(endpoint, timeout=2):
    return {"endpoint": endpoint, "bearerToken": TOKEN, "timeoutSeconds": timeout}


class Receiver:
    def __init__(self, status=204, location=None, delay=0):
        self.requests = []

        receiver = self

        class Handler(BaseHTTPRequestHandler):
            def do_POST(self):
                body = self.rfile.read(int(self.headers.get("Content-Length", "0")))
                receiver.requests.append({
                    "path": self.path,
                    "headers": dict(self.headers.items()),
                    "body": body,
                })
                if delay:
                    time.sleep(delay)
                self.send_response(status)
                if location is not None:
                    self.send_header("Location", location)
                self.end_headers()

            def log_message(self, _format, *_args):
                pass

        self.server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        self.server.daemon_threads = True
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)

    @property
    def endpoint(self):
        return f"http://127.0.0.1:{self.server.server_port}/api/usage"

    def __enter__(self):
        self.thread.start()
        return self

    def __exit__(self, *_args):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join(timeout=2)


class SlowHeaderReceiver:
    def __init__(self, byte_delay=0.015, slow_bytes=100):
        self.byte_delay = byte_delay
        self.slow_bytes = slow_bytes
        self.request_received = threading.Event()
        self.peer_closed = threading.Event()
        self.response_finished = threading.Event()
        self.handler_finished = threading.Event()
        self.connection = None

        receiver = self

        class Handler(BaseHTTPRequestHandler):
            def do_POST(self):
                self.rfile.read(int(self.headers.get("Content-Length", "0")))
                receiver.request_received.set()
                receiver.connection = self.connection
                response = (
                    b"HTTP/1.1 204 No Content\r\nX-Slow: "
                    + b"x" * receiver.slow_bytes
                    + b"\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"
                )
                try:
                    for byte in response:
                        self.wfile.write(bytes((byte,)))
                        self.wfile.flush()
                        time.sleep(receiver.byte_delay)
                    receiver.response_finished.set()
                except OSError:
                    receiver.peer_closed.set()
                finally:
                    receiver.handler_finished.set()

            def log_message(self, _format, *_args):
                pass

        self.server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        self.server.daemon_threads = True
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)

    @property
    def endpoint(self):
        return f"http://127.0.0.1:{self.server.server_port}/api/usage"

    def __enter__(self):
        self.thread.start()
        return self

    def __exit__(self, *_args):
        self.server.shutdown()
        if not self.handler_finished.wait(timeout=1):
            if self.connection is not None:
                try:
                    self.connection.shutdown(2)
                except OSError:
                    pass
                self.connection.close()
            self.handler_finished.wait(timeout=1)
        self.server.server_close()
        self.thread.join(timeout=2)


def set_stdin(data):
    return patch("sys.stdin", io.TextIOWrapper(io.BytesIO(data), encoding="utf-8"))


def _hold_pipe_without_reading(connection, ready_event):
    ready_event.set()
    time.sleep(2)
    connection.close()


class SenderTestCase(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(sender, "bridge.codexbar_sender must implement the sender contract")
        for name in ("DeliveryError", "load_config", "main", "normalize_event", "send_usage"):
            self.assertTrue(callable(getattr(sender, name, None)), f"sender is missing {name}")


class NormalizeEventTests(SenderTestCase):
    def test_normalizes_fractional_usage_and_timestamp_to_utc_without_identity(self):
        normalized = normalize_event(event(
            secondaryUsagePercent=0.5,
            secondaryWindowMinutes=10080,
            secondaryResetAt="2026-10-04T17:00:00+07:00",
            account="private@example.test",
            unknownField="ignored",
        ))

        self.assertEqual(normalized, {
            "schemaVersion": 1,
            "type": "usage",
            "provider": "codex",
            "updatedAt": "2026-10-04T03:00:00Z",
            "primary": {"usedPercent": 0, "windowMinutes": 300, "resetsAt": "2026-10-04T08:00:00Z"},
            "secondary": {"usedPercent": 50, "windowMinutes": 10080, "resetsAt": "2026-10-04T10:00:00Z"},
        })
        self.assertNotIn("account", normalized)

    def test_accepts_both_supported_providers(self):
        for provider in ("claude", "codex"):
            with self.subTest(provider=provider):
                self.assertEqual(normalize_event(event(provider=provider))["provider"], provider)

    def test_normalizes_integer_zero_and_one_usage_fractions(self):
        for fraction, expected in ((0, 0), (1, 100)):
            with self.subTest(fraction=fraction):
                try:
                    normalized = normalize_event(event(usagePercent=fraction))
                except Exception as error:
                    self.fail(f"integer usage fraction raised {type(error).__name__}")
                self.assertEqual(normalized["primary"]["usedPercent"], expected)

    def test_rejects_wrong_event_name(self):
        with self.assertRaises(ValueError):
            normalize_event(event(event="quota_reached"))

    def test_rejects_unknown_provider(self):
        with self.assertRaises(ValueError):
            normalize_event(event(provider="openai"))

    def test_rejects_non_object_event(self):
        for invalid in ([], "event", None):
            with self.subTest(invalid=invalid), self.assertRaises(ValueError):
                normalize_event(invalid)

    def test_rejects_fraction_outside_zero_to_one(self):
        for fraction in (-0.01, 1.01):
            with self.subTest(fraction=fraction), self.assertRaises(ValueError):
                normalize_event(event(usagePercent=fraction))

    def test_rejects_boolean_and_nonfinite_usage_fractions(self):
        for fraction in (True, False, math.nan, math.inf, -math.inf):
            with self.subTest(fraction=fraction), self.assertRaises(ValueError):
                normalize_event(event(usagePercent=fraction))

    def test_omits_secondary_window_when_secondary_fraction_is_absent(self):
        normalized = normalize_event(event(
            secondaryWindowMinutes=10080,
            secondaryResetAt="2026-10-04T17:00:00+07:00",
        ))
        self.assertNotIn("secondary", normalized)

    def test_omits_window_fields_when_usage_fraction_is_absent(self):
        normalized = normalize_event({
            "event": "usage_updated",
            "provider": "claude",
            "timestamp": "2026-10-04T03:00:00Z",
            "windowMinutes": 300,
            "resetAt": "2026-10-04T08:00:00Z",
        })
        self.assertNotIn("primary", normalized)

    def test_preserves_event_with_no_usage_windows_as_unknown_data(self):
        normalized = normalize_event({
            "event": "usage_updated",
            "provider": "claude",
            "timestamp": "2026-10-04T03:00:00Z",
        })
        self.assertNotIn("primary", normalized)
        self.assertNotIn("secondary", normalized)

    def test_rejects_nonpositive_or_nonfinite_window_minutes(self):
        for minutes in (0, -1, True, math.nan, math.inf, "300"):
            with self.subTest(minutes=minutes), self.assertRaises(ValueError):
                normalize_event(event(windowMinutes=minutes))

    def test_rejects_reset_and_source_timestamps_without_explicit_timezone(self):
        for key, value in (
            ("timestamp", "2026-10-04T03:00:00"),
            ("resetAt", "2026-10-04T08:00:00"),
            ("secondaryResetAt", "2026-10-04T08:00:00"),
        ):
            with self.subTest(key=key), self.assertRaises(ValueError):
                normalize_event(event(**{key: value}))

    def test_rejects_malformed_source_and_reset_dates(self):
        for key, value in (
            ("timestamp", "2026-02-30T03:00:00Z"),
            ("timestamp", "not-a-date"),
            ("resetAt", "2026-02-30T03:00:00Z"),
            ("secondaryResetAt", "not-a-date"),
        ):
            with self.subTest(key=key, value=value), self.assertRaises(ValueError):
                normalize_event(event(**{key: value}))

    def test_timestamp_underflow_is_a_sanitized_cli_error(self):
        invalid_event = {
            "event": "usage_updated",
            "provider": "codex",
            "timestamp": "0001-01-01T00:00:00+07:00",
            "usagePercent": 0,
        }
        script = Path(__file__).parents[1] / "bridge" / "codexbar_sender.py"
        result = subprocess.run(
            [sys.executable, str(script), "--dry-run"],
            input=json.dumps(invalid_event),
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertEqual(result.stderr, "codexbar sender: invalid input or configuration\n")
        self.assertEqual(result.stdout, "")


class ConfigTests(SenderTestCase):
    def write_config(self, directory, value):
        path = Path(directory) / "bridge.json"
        path.write_text(json.dumps(value), encoding="utf-8")
        return path

    def test_loads_valid_config_and_defaults_timeout_to_five_seconds(self):
        with tempfile.TemporaryDirectory() as directory:
            path = self.write_config(directory, {
                "endpoint": "http://device.local/api/usage",
                "bearerToken": TOKEN,
            })
            self.assertEqual(load_config(path), {
                "endpoint": "http://device.local/api/usage",
                "bearerToken": TOKEN,
                "timeoutSeconds": 5,
            })

    def test_accepts_https_receiver_endpoint(self):
        with tempfile.TemporaryDirectory() as directory:
            path = self.write_config(directory, {
                "endpoint": "https://device.example/api/usage",
                "bearerToken": TOKEN,
                "timeoutSeconds": 30,
            })
            self.assertEqual(load_config(path)["timeoutSeconds"], 30)

    def test_rejects_missing_or_malformed_config(self):
        with tempfile.TemporaryDirectory() as directory:
            missing_fields = self.write_config(directory, {"endpoint": "http://device.local/api/usage"})
            self.assertRaises(ValueError, load_config, missing_fields)
            invalid_json = Path(directory) / "invalid.json"
            invalid_json.write_text("{", encoding="utf-8")
            self.assertRaises(ValueError, load_config, invalid_json)
            self.assertRaises(ValueError, load_config, Path(directory) / "absent.json")

    def test_rejects_invalid_endpoints(self):
        invalid_endpoints = (
            "ftp://device.local/api/usage",
            "http:///api/usage",
            "http://user:pass@device.local/api/usage",
            "http://device.local/api/other",
            "http://device.local/api/usage?token=secret",
            "https://device.local/api/usage#section",
            "http://device.local/api/usage?",
            "http://device.local/api/usage#",
            "http://device.local:99999/api/usage",
        )
        with tempfile.TemporaryDirectory() as directory:
            for endpoint in invalid_endpoints:
                with self.subTest(endpoint=endpoint):
                    path = self.write_config(directory, {"endpoint": endpoint, "bearerToken": TOKEN})
                    with self.assertRaises(ValueError):
                        load_config(path)

    def test_rejects_empty_and_control_character_tokens(self):
        for token in ("", "\n", "abc\r\ndef", "abc\x00def", "abc\x7fdef", "tøken"):
            with self.subTest(token=repr(token)), tempfile.TemporaryDirectory() as directory:
                path = self.write_config(directory, {
                    "endpoint": "http://device.local/api/usage",
                    "bearerToken": token,
                })
                with self.assertRaises(ValueError):
                    load_config(path)

    def test_rejects_timeouts_outside_finite_point_one_to_thirty_seconds(self):
        for timeout in (0, 0.099, 30.001, True, math.nan, math.inf, "5"):
            with self.subTest(timeout=timeout), tempfile.TemporaryDirectory() as directory:
                path = self.write_config(directory, {
                    "endpoint": "http://device.local/api/usage",
                    "bearerToken": TOKEN,
                    "timeoutSeconds": timeout,
                })
                with self.assertRaises(ValueError):
                    load_config(path)


class CliTests(SenderTestCase):
    def test_dry_run_needs_no_config_and_prints_normalized_json_without_network(self):
        stdout = io.StringIO()
        stderr = io.StringIO()
        network_attempts = []

        def forbidden_network(*_args, **_kwargs):
            network_attempts.append(True)
            raise AssertionError("dry-run must not open a network connection")

        with set_stdin(json.dumps(event()).encode()), contextlib.redirect_stdout(stdout), \
                contextlib.redirect_stderr(stderr), patch("urllib.request.OpenerDirector.open", forbidden_network):
            result = main(["--dry-run"])

        self.assertEqual(result, 0)
        self.assertEqual(json.loads(stdout.getvalue()), PAYLOAD)
        self.assertEqual(stderr.getvalue(), "")
        self.assertEqual(network_attempts, [])

    def test_rejects_input_over_four_kibibytes(self):
        data = b" " * 4097
        stderr = io.StringIO()
        with set_stdin(data), contextlib.redirect_stderr(stderr):
            result = main(["--dry-run"])
        self.assertEqual(result, 2)
        self.assertNotIn("4097", stderr.getvalue())

    def test_accepts_input_at_four_kibibytes(self):
        data = json.dumps(event()).encode("utf-8")
        self.assertLess(len(data), 4096)
        data += b" " * (4096 - len(data))
        stdout = io.StringIO()
        with set_stdin(data), contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(io.StringIO()):
            result = main(["--dry-run"])
        self.assertEqual(result, 0)
        self.assertEqual(json.loads(stdout.getvalue()), PAYLOAD)

    def test_rejects_invalid_json_and_non_object_json(self):
        for data in (b"not-json", b"[]"):
            with self.subTest(data=data), set_stdin(data), contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(main(["--dry-run"]), 2)

    def test_returns_two_when_delivery_config_is_missing(self):
        with set_stdin(json.dumps(event()).encode()), contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(main([]), 2)


class DeliveryTests(SenderTestCase):
    def test_posts_json_to_receiver_with_expected_path_and_bearer_header(self):
        with Receiver() as receiver:
            send_usage(PAYLOAD, config(receiver.endpoint))

        self.assertEqual(len(receiver.requests), 1)
        request = receiver.requests[0]
        self.assertEqual(request["path"], "/api/usage")
        self.assertEqual(request["headers"]["Content-Type"], "application/json")
        self.assertEqual(request["headers"]["Authorization"], f"Bearer {TOKEN}")
        self.assertEqual(json.loads(request["body"]), PAYLOAD)

    def test_accepts_http_204_as_success(self):
        with Receiver(status=204) as receiver:
            self.assertIsNone(send_usage(PAYLOAD, config(receiver.endpoint)))

    def test_receiver_rejection_returns_sanitized_delivery_error(self):
        with Receiver(status=500) as receiver:
            with self.assertRaises(DeliveryError) as raised:
                send_usage(PAYLOAD, config(receiver.endpoint))

        self.assertEqual(str(raised.exception), "delivery failed")
        self.assertNotIn(TOKEN, str(raised.exception))
        self.assertNotIn(receiver.endpoint, str(raised.exception))
        self.assertEqual(len(receiver.requests), 1)

    def test_cli_returns_one_and_redacts_delivery_failure(self):
        with Receiver(status=500) as receiver, tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "bridge.json"
            path.write_text(json.dumps(config(receiver.endpoint)), encoding="utf-8")
            stderr = io.StringIO()
            with set_stdin(json.dumps(event()).encode()), contextlib.redirect_stderr(stderr):
                result = main(["--config", str(path)])

        self.assertEqual(result, 1)
        self.assertEqual(stderr.getvalue(), "codexbar sender: delivery failed\n")
        self.assertNotIn(TOKEN, stderr.getvalue())
        self.assertNotIn(receiver.endpoint, stderr.getvalue())

    def test_redirect_is_rejected_before_destination_receives_bearer_request(self):
        with Receiver() as destination:
            with Receiver(status=302, location=destination.endpoint) as source:
                with self.assertRaises(DeliveryError):
                    send_usage(PAYLOAD, config(source.endpoint))

        self.assertEqual(len(source.requests), 1)
        self.assertEqual(destination.requests, [])

    def test_timeout_returns_sanitized_delivery_error(self):
        with Receiver(delay=0.3) as receiver:
            started = time.monotonic()
            with self.assertRaises(DeliveryError) as raised:
                send_usage(PAYLOAD, config(receiver.endpoint, timeout=0.1))
            elapsed = time.monotonic() - started

        self.assertLess(elapsed, 1)
        self.assertEqual(str(raised.exception), "delivery failed")
        self.assertNotIn(TOKEN, str(raised.exception))
        self.assertNotIn(receiver.endpoint, str(raised.exception))

    def test_timeout_is_an_overall_deadline_and_stops_slow_header_transport(self):
        before = {process.pid for process in multiprocessing.active_children()}
        with SlowHeaderReceiver() as receiver:
            started = time.monotonic()
            failed = False
            try:
                send_usage(PAYLOAD, config(receiver.endpoint, timeout=0.6))
            except DeliveryError as error:
                failed = str(error) == "delivery failed"
            elapsed = time.monotonic() - started

            self.assertTrue(receiver.request_received.wait(0.1), "receiver never got the POST")
            self.assertTrue(failed, "a slow but progressing response must time out")
            self.assertLess(elapsed, 1.2, "total delivery duration exceeded the configured deadline")
            self.assertTrue(receiver.handler_finished.wait(1), "timed-out request left receiver-side transport active")
            self.assertTrue(receiver.peer_closed.is_set(), "sender did not close the stalled response connection")
            self.assertFalse(receiver.response_finished.is_set(), "sender waited for all slow header bytes")

        leaked = [process for process in multiprocessing.active_children() if process.pid not in before]
        self.assertEqual(leaked, [], "sender left a transport worker process running")

    def test_ipc_transfer_obeys_deadline_with_held_reader_and_one_mib_token(self):
        before = {process.pid for process in multiprocessing.active_children()}
        ready_event = multiprocessing.get_context("spawn").Event()
        large_config = {
            "endpoint": "http://127.0.0.1:1/api/usage",
            "bearerToken": "x" * (1024 * 1024),
            "timeoutSeconds": 0.5,
        }
        started = time.monotonic()
        transfer_started = threading.Event()
        holding_worker = functools.partial(_hold_pipe_without_reading, ready_event=ready_event)
        send_frame = sender._send_pipe_frame_before

        def track_transfer(connection, payload, deadline):
            transfer_started.set()
            return send_frame(connection, payload, deadline)

        with patch.object(sender, "_deliver_in_worker", holding_worker), \
                patch.object(sender, "_send_pipe_frame_before", track_transfer):
            with self.assertRaises(DeliveryError):
                send_usage(PAYLOAD, large_config)
        elapsed = time.monotonic() - started

        self.assertTrue(ready_event.is_set(), "the held-reader worker did not start")
        self.assertTrue(transfer_started.is_set(), "the sender skipped the request-transfer phase")
        self.assertLess(elapsed, 1.2, "request IPC blocked past its monotonic deadline")
        leaked = [process for process in multiprocessing.active_children() if process.pid not in before]
        self.assertEqual(leaked, [], "deadline failure left the held-reader worker alive")

    def test_sigterm_cancels_cli_and_closes_worker_transport(self):
        script = Path(__file__).parents[1] / "bridge" / "codexbar_sender.py"
        with SlowHeaderReceiver(byte_delay=0.02, slow_bytes=500) as receiver, \
                tempfile.TemporaryDirectory() as directory:
            config_path = Path(directory) / "bridge.json"
            config_path.write_text(json.dumps(config(receiver.endpoint, timeout=8)), encoding="utf-8")
            process = subprocess.Popen(
                [sys.executable, str(script), "--config", str(config_path)],
                stdin=subprocess.PIPE,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            cancelled = threading.Event()

            def supply_event():
                try:
                    process.stdin.write(json.dumps(event()).encode())
                    process.stdin.close()
                except OSError:
                    pass

            def cancel_after_post():
                if receiver.request_received.wait(timeout=3):
                    process.terminate()
                    cancelled.set()

            canceller = threading.Thread(target=cancel_after_post, daemon=True)
            writer = threading.Thread(target=supply_event, daemon=True)
            canceller.start()
            writer.start()
            try:
                process.wait(timeout=5)
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait(timeout=2)
                canceller.join(timeout=1)
                writer.join(timeout=1)

            return_code = process.returncode
            peer_closed = receiver.peer_closed.wait(timeout=1)
            handler_finished = receiver.handler_finished.wait(timeout=1)
            response_finished = receiver.response_finished.is_set()

        self.assertTrue(cancelled.is_set(), "sender never started its request")
        self.assertEqual(return_code, 128 + signal.SIGTERM)
        self.assertTrue(peer_closed, "SIGTERM left a child transport reading the response")
        self.assertTrue(handler_finished, "SIGTERM left receiver-side transport activity running")
        self.assertFalse(response_finished, "orphaned transport consumed all slow header bytes")

    def test_send_usage_validates_direct_config_before_network(self):
        with self.assertRaises(ValueError):
            send_usage(PAYLOAD, {"endpoint": "http://device.local/api/usage?secret=x", "bearerToken": TOKEN})


if __name__ == "__main__":
    unittest.main()
