# Usage sender and receiver protocol

This protocol carries the latest Claude or Codex usage snapshot from the CodexBar macOS app to an ESP32 on the local network. The sender is source-only; the device receiver is a separate firmware task.

## CodexBar input

The app invokes `bridge/codexbar_sender.py` directly and writes one `usage_updated` JSON object to stdin. The sender reads at most 4096 bytes. CodexBar's documented fields are `event`, `provider`, `timestamp`, `usagePercent`, `windowMinutes`, `resetAt`, and the matching `secondary*` fields. Unknown fields, including account identity, are ignored.

The `provider` must be `claude` or `codex`. Usage values are fractions from 0 to 1. Timestamps must include an explicit timezone. The sender normalizes dates to UTC and percentages to 0–100. It validates numeric values as finite and rejects malformed dates before sending.

## Receiver request

The receiver accepts:

```text
POST /api/usage
Content-Type: application/json
Authorization: Bearer <device token>
```

Any HTTP 2xx response means the snapshot was accepted. Redirects and other status codes are failures. The sender makes one request per hook invocation, uses the configured timeout (0.1–30 seconds, default 5), and does not retry. It does not print response bodies or include the token or endpoint URL in errors.

The JSON body has this shape:

```json
{
  "schemaVersion": 1,
  "type": "usage",
  "provider": "codex",
  "updatedAt": "2026-10-04T03:00:00Z",
  "primary": {
    "usedPercent": 42,
    "windowMinutes": 300,
    "resetsAt": "2026-10-04T08:00:00Z"
  },
  "secondary": {
    "usedPercent": 71.5,
    "windowMinutes": 10080,
    "resetsAt": "2026-10-09T17:00:00Z"
  }
}
```

`updatedAt` comes from the source event's `timestamp`; it is not the time the sender delivered the request. Each window's `usedPercent` is 0–100. `windowMinutes` is present only when CodexBar supplies it, and `resetsAt` is the UTC form of `resetAt` or `secondaryResetAt`. If a usage fraction is missing, that whole window is omitted. A reported zero remains zero. No quota, window, timestamp, account, or identity is invented or forwarded.

The checked-in [input example](../examples/usage-event.json) and [normalized payload](../examples/usage-response.json) are simulated values. The example device token and endpoint in [bridge-config.example.json](../examples/bridge-config.example.json) are placeholders.

## Local configuration

`--config FILE` reads JSON with `endpoint`, `bearerToken`, and optional `timeoutSeconds`. The endpoint must be HTTP or HTTPS with the exact `/api/usage` path and no credentials, query, or fragment. Store the real file outside the repository with owner-only permissions; never place its token in a hook argument, source file, or log.

The sender supports `--dry-run`, which reads and validates the event and prints the normalized JSON without loading a config or opening a network connection.

## Independent feeds

The Mac sends usage only. Weather is fetched by the ESP32 from its configured source; stocks are fetched independently from the Google Sheets endpoint described in the [Sheets guide](../integrations/google-sheets/README.md), with the [stocks response fixture](../examples/stocks-response.json) as its example. Neither feed passes through CodexBar or this sender.

## Device receiver and configuration portal

The built firmware implements `POST /api/usage`: a separate `Authorization: Bearer <usageToken>` and `Content-Type: application/json` are required. Valid usage is 204; wrong credentials are 401; invalid JSON/schema is 400; a body over 4096 bytes is 413; a different content type is 415. An absent primary or secondary window stays absent. Older updates cannot overwrite newer provider data. `windowMinutes` is any positive finite number, including fractional minutes.

`GET /`, `GET /api/config`, and `POST /api/config` require HTTP Basic username `admin` with the device's randomly generated setup/AP password. A usage bearer does not grant settings access, and Basic does not grant usage ingestion. POST configuration requires JSON and at most 8192 bytes. Omitted fields remain unchanged. Config is validated, serialized, restored and compared across every field, then committed as an NVS blob before atomic application (88 KiB NVS allows old/new bounded payloads during replacement); persistence failure returns503 without changing active state. Internal saved JSON may use up to32768 bytes because validated field contents can require JSON escaping.

GET configuration uses the core redacted representation: no SSID, Wi-Fi password, setup password, usage token or headers; URL queries/fragments are removed. Portal secret/URL/SSID inputs remain blank and are omitted on an unchanged save. Explicit clear controls remove Wi-Fi credentials or source URLs, while an entered empty headers object clears headers. No permissive CORS headers are provided.

Public Weather readback may additionally contain numeric `weather.latitude` and `weather.longitude`. They are derived together only from a validated HTTPS `api.open-meteo.com/v1/forecast` endpoint on the default HTTPS port (or explicit 443), with one finite in-range latitude and longitude. Malformed numbers, duplicate coordinates, encoded parameter names, credentials, fragments, other hosts/paths and other ports suppress both values. This deliberately conservative projection does not expose any other query values or headers. These fields are read-only metadata: they are rejected in configuration patches and never persisted. The portal hydrates friendly coordinates without regenerating or submitting the saved URL. Older redacted responses without coordinates still permit label, refresh and Advanced-only saves that preserve the endpoint; explicit friendly provider/coordinate edits require valid coordinates.

Device sources have a single 10-second total budget including asynchronous DNS, nonblocking connect/TLS, headers, body and up to4 redirect hops. Responses are at most16384 decoded body bytes; headers/chunk framing are additionally bounded by32768 wire bytes per hop. Configured headers are permanently removed after any cross-origin redirect, and HTTPS downgrade is rejected. Verified certificate bundle/hostname checks remain enabled; HTTPS waits for valid SNTP time. Root-relative redirects are resolved; other relative forms are rejected conservatively. Failures retain same-source data as stale; source changes reset that cache to unknown and invalidate old generations.

Stocks additionally accept `provider` (`json` legacy default or `yahoo`),
`chartRange` (`1d` or `5d`) and up to five `watchlist` objects containing canonical
`symbol` and boolean `enabled`. Thai symbols end in `.BK`. Yahoo refresh is
300–86400 seconds. Each enabled symbol has its own bounded request through the
same verified transport. These fields are public readback metadata, are included
in NVS roundtrip verification and source generation identity, and are rejected on
weather sources. See [stock setup and percentage semantics](stocks.md).

The UI uses Bangkok time and displays unknown time when SNTP has not established a usable clock. Display age is monotonic receipt age, not proof of a current Sheet quote. Mac usage and ESP32 weather/stocks remain independent.


### Weather presentation fields

Weather mappings add optional `isDay` (`current.is_day`), `hourlyWeatherCodes`
(`hourly.weather_code`), and `hourlyIsDay` (`hourly.is_day`). Existing stored
mappings remain valid: omitted mappings use defaults, and absent/null source
values remain unknown. Day flags accept JSON booleans or integer 0/1; other
values reject the response. Forecast codes use the same WMO code validation as
`weatherCode`. Mapping objects support all eleven weather keys.

The ESP32 retains at most four forecast entries, keeping time, temperature,
rain probability, weather code, and day flag at the same source index. When
source `updatedAt` and a forecast time are known UTC instants, hours before the
observation are skipped. Equal observation times are retained. Known retained
times must increase; invalid timestamps reject the response. Absent times or
local timestamps without a source offset remain unknown and preserve the
legacy source-index order. Missing or short companion arrays produce unknown
values, without borrowing another hour's data. Only the selected slice's
measurement values are validated, matching the existing bounded parser;
scanned timestamps are validated while locating that slice.

`GET /api/network` exposes `weather.updatedAt` as UTC epoch seconds,
`weather.utcOffsetSeconds` as the source offset, and `weather.isDay` as a JSON
boolean; each is null when unknown. `weather.ageMs` remains receipt age and
must not be used as evidence that the observation itself is current.
