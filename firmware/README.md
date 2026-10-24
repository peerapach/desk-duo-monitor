# Desk Duo firmware (portrait CYD)

For Thai/US stocks and sparklines, see the [Yahoo Finance guide](../docs/stocks.md). Existing JSON endpoint settings remain valid and are not switched to another provider automatically.

Desk Duo 2.2.4 targets ESP32-2432S028R / ESP32-D0WD-V3 with 4 MiB flash, no PSRAM, and a 240×320 display. It uses LVGL 9.2.2 and ESP-IDF 5.5.3 through espressif32 6.13.0. Local-network OTA uses Basic administrator authentication and boot rollback. Build and native tests do not establish physical-device behavior or live-source accuracy.

## Build and test on macOS

Run from the repository root:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements-build.txt
.venv/bin/python scripts/pio.py run -d firmware -e cyd
.venv/bin/python scripts/test_firmware.py
.venv/bin/python -m unittest discover -s tests -p 'test_*.py' -v
node --test tests/stocks_endpoint.test.cjs tests/portal.test.cjs
.venv/bin/python scripts/package_firmware.py
```

Run native UI tests with the LVGL dependencies downloaded by the target build; no device is required:

```sh
.platformio/packages/tool-cmake/bin/cmake -S tests/native_ui -B .platformio/native-ui-regression -DCMAKE_BUILD_TYPE=Debug
.platformio/packages/tool-cmake/bin/cmake --build .platformio/native-ui-regression -j 8
.platformio/packages/tool-cmake/bin/ctest --test-dir .platformio/native-ui-regression --output-on-failure
```

The UI regression uses simulated runtime/GPIO and actual `ui.cpp`. It tests all four bottom buttons, LAN IP visibility, token changes while Setup is open, layout/scroll changes, hide/reopen behavior, fractional usage windows, reset dates in 2100, automatic rotation that skips Setup, and the 15-second hold after a tap. It writes PPM screenshots to the build directory.

`scripts/pio.py` applies system trust only to the build process. It does not disable TLS verification or change system Python/CA settings. `.platformio/`, `firmware/.pio/`, `managed_components/`, and `artifacts/` are ignored; `dependencies.lock` pins managed components.

Packaging creates `artifacts/desk-duo-cyd.bin`, `.sha256`, `.json`, `desk-duo-app.bin`, and notices. It validates build-metadata offsets, dual-OTA partitions, sizes, SHA256, image checksum/identity, and the bytes of all four segments. The packaging script does not access serial devices.

Additional offline portal DOM tests require an existing Playwright/Chromium installation; tests do not install or download browsers. Set `NODE_PATH` to a runtime containing `playwright`. If necessary, set `PLAYWRIGHT_CHROMIUM_EXECUTABLE` to the absolute path of an installed Chromium executable, then run `node --test tests/portal_dom.cjs`. Tests block network requests and use local fixtures. Bilingual captures are written to `.platformio/native-ui-regression/portal-language-{th,en}.png`.

## Web admin

The six sections are **Display**, **AI usage**, **Weather**, **Stocks**, **Wi-Fi**, and **Firmware**. Wide screens use a sidebar; mobile screens use a section selector. Drafts survive language and section changes. Each section has its own save/cancel controls and separates API-reported saved values from draft edits. Status and errors reflect device responses.

- **Display:** language, 5–100% base brightness, and scheduled brightness. Day uses the full base; night uses half, with a 5% minimum, according to the ESP32 clock.
- **AI usage:** both Codex windows or Weekly only; replace the usage token without revealing its saved value.
- **Weather:** friendly Open-Meteo location/coordinate fields plus Advanced controls for custom endpoints, headers, and mappings.
- **Stocks:** Yahoo/legacy source selection and Today / 5 days chart range.
- **Wi-Fi:** saved SSID and secret-presence status. A blank password field preserves the saved password.
- **Firmware:** API-reported version/capacity and the selected file's name/size before an explicitly initiated, authenticated upload.

## Device setup

The initial screen shows the `DeskDuo-xxxxxx` SSID, a random 12-character WPA2 password, `http://192.168.4.1`, and a random 32-hex-character usage bearer token. Join the AP and open the URL. HTTP Basic uses username `admin` and the same password as the AP. The portal uses local HTTP; use the password-protected AP or a trusted LAN.

Press and release BOOT to reveal or hide Setup without clearing settings. The display shows the LAN IP after Wi-Fi connects. Use that IP in the bridge configuration when the Mac shares the LAN; use `192.168.4.1` when connected to the AP. This firmware has no mDNS hostname.

### Configuration and readback

Configure Wi-Fi, weather/stock URLs, JSON headers, field mappings, refresh intervals of 30–86400 seconds, automatic rotation (`0` to disable, otherwise 5–300 seconds), and the usage token in the portal. Yahoo stocks require at least 300 seconds.

Blank SSID/password/token/URL/header replacement fields preserve existing values. Use explicit clear controls to clear Wi-Fi or a URL; headers `{}` clears all headers. Administrator settings readback exposes the saved SSID, redacted URLs without query/fragment, and presence flags for Wi-Fi password, usage token, and headers. A saved SSID does not prove current radio association. Secrets themselves remain hidden.

Replacement fields stay blank, and only changed fields from a successfully loaded snapshot are submitted. Unchanged forms do not POST. Failed initial reads show unknown state and disable saving. If saving succeeds but readback fails, the UI reports the save and requires a successful reload before further edits. Metadata exists only in the public response and does not change Config or its NVS bytes. Codex display preferences use a separate read/save flow.

Select **Thai / English** in Display to change both device and web interfaces. The default is Thai until a language is saved. The browser previews a selected language before saving and shows the saved language separately. After a successful save, the device updates its current page and retains the language after reboot. User-entered locations, SSIDs, and stock names are not translated.

Select **5h + Weekly** (default) or **Weekly only** in AI usage. Saving takes effect immediately without rebooting or changing Wi-Fi/source settings. Saving remains disabled until current settings load successfully.

### Brightness and schedules

Base brightness is 5–100%, default 100%. On the web, move the slider and choose **Save display settings** to submit only changed fields. Scheduled target values never overwrite the saved base. Readback distinguishes saved mode/language/base/Auto/day/night settings from target brightness and last-applied brightness. Last-applied is the latest accepted hardware request, not measured light output; `null` means unknown.

Enable **Auto brightness** and set distinct day/night start times in `HH:MM`. Defaults are 06:00/18:00. Scheduling uses the ESP32's `ICT-7` timezone (UTC+07:00), not the browser clock. Day uses the full base. Night uses half, rounds .5 upward, and applies a minimum of 5%; a base of 11 therefore gives 6 at night. Day periods may cross midnight.

When Auto is enabled before the clock is ready, the device uses the saved base and reports `clock_wait`, then adjusts when time becomes usable. Clock/phase changes do not write NVS. Defaults and migration from old preferences leave Auto disabled, preserving manual brightness. Disable Auto for fixed brightness.

The on-device Setup slider also edits the base. Dragging previews the current schedule-derived level without writing NVS. Release saves the base once under the shared Settings lease, reevaluating the latest clock and schedule. If preview or the required release-time application fails, the new base is not saved. Cancellation, leaving Setup, BOOT, language changes, or save failures restore the level derived from the latest committed base/schedule and current time. Setup shows base/applied/Auto state; enable Auto and edit schedule times through the web.

While open, the web reads status approximately every 15 seconds without writing settings or overwriting drafts, including language/location edits. If a POST succeeds but GET readback fails, saved values become unknown until a valid GET permits saving again. Failed POSTs preserve the previously known snapshot. Reads started before a POST cannot restore old values as current.

### Display API and storage

`GET /api/display` and `POST /api/display` require Basic admin authentication. GET returns saved fields `codexUsageMode`, `language`, `brightnessPercent`, `brightnessAuto`, `brightnessDayStart`, and `brightnessNightStart`, plus read-only `brightnessTargetPercent`, `brightnessEffectivePercent`, and `brightnessPhase` (`manual`, `day`, `night`, `clock_wait`).

POST accepts at most 512 JSON bytes, for example `{"brightnessPercent":50}`, `{"brightnessAuto":true}`, `{"brightnessDayStart":"07:15","brightnessNightStart":"19:45"}`, or multiple saved fields together. Brightness must be an integer 5–100; Auto must be boolean. Times must be distinct ASCII `HH:MM` values after merging, even when Auto is off. Invalid values/types, unknown/read-only keys, duplicate keys, and empty patches are rejected. Sparse updates preserve omitted fields. Shared settings admission blocks both web and device saves during OTA (HTTP 503). If supplied, Origin must match the device's HTTP Host.

Display preferences are separate from the Config blob: NVS namespace `deskduo`, scalar u32 key `ui_pref_v3`. Encoding: mode bit 0, language bit 1, base bits 2–8, Auto bit 9, day-start minutes bits 10–20, night-start minutes bits 21–31. Missing, invalid, or wrong-type v3 falls back to valid v2 (preserving mode/language/base with Auto off and 06:00/18:00), then `ui_pref`, `codex_layout`, and defaults, without migration writes. Old values and the Config blob are unchanged.

A single scalar holds all six fields. The pinned ESP-IDF writes NVS during set; commit is a no-op afterward. Preferences become active together only after set/commit succeed. A reported commit failure preserves active preferences, even if set already persisted the complete scalar.

Both usage modes show source-provided durations/resets. Codex 10080-minute windows are labeled Weekly. Weekly selection prefers a 10080-minute primary, then secondary; otherwise it accepts an undated secondary, or an undated primary only when it is the sole window. Other durations are not relabeled Weekly. Without an eligible window, the UI shows unknown/`--` and hides secondary values/reset/bar. Telemetry is unchanged; Claude keeps its source durations. Weather and Stocks have no mini sprites. Stocks shows four sparkline rows with Today / 5 days controls.

Configuration is validated and round-tripped across every field before committing an NVS blob and replacing active settings. The 88 KiB partition accommodates old/new payloads during replacement. Flash-save failure returns HTTP 503 and leaves active settings unchanged. Normal restart retains settings. Corrupt NVS stops startup rather than silently erasing secrets.

## Usage from the Mac

The [bridge guide](../docs/codexbar.md) sends Claude/Codex only; no CodexBar CLI is needed. Store `http://<LAN-IP>/api/usage` and the device token in a private file outside the repo. After installation, a sample request is:

```sh
curl --fail-with-body --request POST \
  --header 'Content-Type: application/json' \
  --header 'Authorization: Bearer REPLACE_WITH_DEVICE_TOKEN' \
  --data-binary @examples/usage-response.json \
  http://192.168.4.1/api/usage
```

Examples contain simulated values. Usage Bearer credentials cannot edit settings; Basic admin credentials cannot ingest usage.

## Weather and source configuration

A separate network task fetches the two sources sequentially, independently of the display. For Apps Script stocks, use the deployed URL from the [Sheets guide](../integrations/google-sheets/README.md), including `accessToken` if configured. Apps Script often redirects to `script.googleusercontent.com`. Firmware follows at most four redirects within one 10-second budget. A cross-origin redirect permanently removes configured headers, even if a later hop returns to the original origin. HTTPS-to-HTTP downgrade is rejected.

The Weather editor builds an Open-Meteo endpoint when provider/coordinate fields are explicitly edited. Custom URL, headers, and mappings remain in Advanced and are preserved during reads and unrelated edits. Example for Bangkok:

```text
https://api.open-meteo.com/v1/forecast?latitude=13.7563&longitude=100.5018&current=temperature_2m,relative_humidity_2m,weather_code&hourly=temperature_2m,precipitation_probability&forecast_hours=4&timezone=Asia%2FBangkok
```

In the [Open-Meteo API](https://open-meteo.com/en/docs), `forecast_hours=4` starts at the current hour and returns four periods: at 12:45, for example, 12:00–15:00. `forecast_days=1` with a timezone starts at midnight and is unsuitable for this example. See the [simulated response](../examples/weather-response.json). The UI shows current conditions and the next three periods in the response, skipping known hourly times before the source observation's `updatedAt`.

Default mappings are in the [example configuration](../examples/device-config.example.json). `current.is_day`, `hourly.weather_code`, and `hourly.is_day` are optional. Missing source values leave condition/day status unknown rather than guessing a moon. The parser normalizes observation `updatedAt` to UTC, then displays it using the source UTC offset. Receipt age is separate: a newly received response does not prove a fresh observation. Local times without a UTC offset remain unknown. HTTP bodies are capped at 16384 bytes; mappings are editable in the portal.

HTTPS uses the certificate bundle and verifies hostname/SNI. It waits for usable SNTP time rather than disabling certificate checks. UI time and brightness schedules use `ICT-7`; weather uses its source UTC offset. The clock shows `--:--` when unknown. The UI displays received values or `--`/unknown, never fabricated readings. Receipt age is time since successful delivery, not source age. Stale means the latest fetch failed or data exceeded two refresh intervals. Missing prices do not become zero.

## Display and hardware validation

The 240×320 black layout has a 24px header, 232px content area, LAN IP strip at y=256, and 60×44 Duo/Weather/Stocks/Setup buttons at y=276. Duo uses actual provider windows and leaves missing windows unknown. Weather separates observation time/offset from receipt age and marks stale observations. Condition/day artwork requires supporting source values. Stocks uses received quotes, timestamps, stale state, and sparklines. Setup edits base brightness and language/schedule preferences through the API; target brightness is not measured light output.

See [source PNGs and the asset pipeline](assets/README.md). Device artwork is compiled into flash as RGB565; runtime does not decode full PNGs. The portal embeds PNG thumbnails as data URLs for offline use. Target/native renderers share fonts.

LCD: SPI2 MOSI13, MISO12, CLK14, CS15, DC2, BL21, 40 MHz, RGB565. A single 240×24×2-byte partial buffer is used, without a duplicate full-screen buffer. XPT2046 touch: SPI3 CLK25, MOSI32, MISO39, CS33, IRQ36, 1 MHz, median sampling. The `touch` configuration supports `swapAxes`, `invertX`, `invertY`, `minX`, `maxX`, `minY`, and `maxY`.

Validate all four display/touch corners, portrait MADCTL 0x48, red/blue channels, contrast/inversion, BOOT reveal, page/animation responsiveness during slow sources/timeouts, low heap during HTTPS+portal activity, NVS save/restart, wrong Basic/Bearer credentials, invalid bodies, in-flight source changes, missing SNTP, oversized responses, and cross-origin redirects on actual hardware.

Thai font/shaping integration originated in tamaclaude; generated fonts and their current source inputs are documented in [font sources](main/fonts/source/README.md). Montserrat supplies other Latin text. Unsupported characters outside available ASCII/Thai/degree coverage render as `?`. See [third-party notices](../THIRD_PARTY_NOTICES.md).

## OTA and USB migration while preserving settings

Dual OTA was introduced in 2.0.0 and remains in 2.2.4:

| Partition | Offset | Size |
| --- | --- | --- |
| NVS | `0x9000` | `0x16000` |
| PHY | `0x1f000` | `0x1000` |
| OTA data | `0x20000` | `0x2000` |
| OTA0 | `0x30000` | `0x1e0000` |
| OTA1 | `0x210000` | `0x1e0000` |

NVS/PHY positions, sizes, and the NVS schema remain unchanged, preserving Wi-Fi/admin/token/endpoint/touch settings.

Factory-layout boards require a one-time validated segmented USB migration. **Never write merged `desk-duo-cyd.bin` at 0x0 on an already configured board.** That image is for initial installation; its padding erases existing configuration. `scripts/package_firmware.py` creates app-only, bootloader, partition table, erased OTA initialization, manifest, and merged images without serial access.

Start with a dry run. Without `--flash`, the tool does not open a device even if `--port` is supplied:

```sh
.venv/bin/python scripts/package_firmware.py
.venv/bin/python scripts/migrate_ota.py
```

By default, the tool validates the chip/flash and reads a fresh 4 MiB backup into a timestamped directory (permissions 0700, files 0600), recording SHA256 and recovery metadata before writing segments:

```sh
.venv/bin/python scripts/migrate_ota.py --port PORT --flash
```

To intentionally skip backup creation, use `--skip-backup`. This mode does not call `read_flash` or create a backup:

```sh
.venv/bin/python scripts/migrate_ota.py --port PORT --flash --skip-backup
```

Only bootloader `0x1000`, partitions `0x8000`, OTA data `0x20000`, and app `0x30000` are written. Erase ranges are checked against NVS/PHY and esptool verifies the writes. The tool does not use `erase_flash`, modify eFuses, or change credentials.

After migration, sign in with the existing Basic admin credentials, select `desk-duo-app.bin`, and explicitly choose **Upload firmware**. Selecting a file does not upload it. 100% progress means bytes have been sent, not that SDK validation finished. After validation the device restarts; the UI attempts reconnect up to 30 times, using 2-second fetch timeouts and 2-second pauses. Check the display or reload if the new slot is not confirmed.

### OTA API and worker

`GET /api/firmware` requires Basic admin and returns version, capacity, busy, runningPartition, and bootState (ESP-IDF enum), without secrets. `POST /api/firmware` accepts an application-only image with Basic admin, `Content-Type: application/octet-stream`, `X-DeskDuo-Update: 1`, and a fixed positive Content-Length within slot capacity. Usage Bearer credentials have no access. A supplied Origin must exactly equal `http://` plus Host. There is no permissive CORS.

The SDK validates the ESP32 image, project `desk_duo`, complete checksum, and image length against received bytes before changing boot slots. This is an admin-authorized firmware update, not publisher-signature validation.

The worker is separate from the HTTP server, using a 4 KiB buffer, 10-second idle timeout, and 120-second total deadline. The server allows four client sockets with LRU reclamation of idle sessions; active async uploads are protected from eviction. Usage/status and LVGL remain serviced. Settings POSTs return HTTP 503 during maintenance; station reconfiguration/reconnect uses the same atomic admission. Source polling pauses during OTA, although already-started requests may finish within their original deadline. The worker does not call LVGL.

Rollback is enabled in defaults and effective sdkconfig. A trial image is confirmed only after runtime/display/input/UI/AP/portal/network-task initialization and the first UI service iteration. Confirmation does not wait for internet access or station association. Reset/crash before confirmation returns to the previous slot.

### Recovery and rollback testing

When restoring a complete backup, first verify SHA256 against its private `recovery.json`, then intentionally restore the old layout at 0x0 with esptool. Full-backup restoration is recovery, not migration. Without a backup, use validated segmented migration to install a known-good release over USB while preserving NVS/PHY. The OTA0 serial app offset is `0x30000`; writing only the app does not update OTA boot selection. Prefer Wi-Fi OTA or segmented recovery that resets OTA data.

The controlled rollback probe uses separate source/build/output, never calls NVS/config APIs, and resets itself before boot confirmation:

```sh
.venv/bin/python scripts/pio.py run -d tests/rollback_probe -e rollback_probe
```

Its image is `tests/rollback_probe/.pio/build/rollback_probe/firmware.bin` (project `desk_duo`, version `rollback-probe`). **Do not distribute it as a release or flash it over USB.** Prepare USB recovery and verify a known-good app before uploading the probe through authenticated OTA. Confirm that the original version/slot returns. Hardware checks must also cover retained settings after migration, successful upload/reconnect/usage hooks, and truncated uploads leaving the boot slot unchanged.

## Network diagnostics

Administrator `GET /api/network` reads the latest bounded weather attempt and accepted weather status. Usage Bearer credentials cannot read it. The portal's **Refresh status** button only reads diagnostics; it does not trigger a source fetch. A failed status read does not block saving settings. A committed settings save invalidates prior status, including in-flight responses; read again for current settings.

Diagnostics live in RAM and reset on reboot. They contain generation and monotonic attempt/next-due times, station/clock readiness (an epoch threshold, not proof of SNTP synchronization), whitelisted outcomes/parser reasons, observed numeric HTTP/TLS/socket errors, bounded byte/redirect counts, and internal 8-bit heap measurements. HTTP/TLS observations describe the latest attempted redirect hop; unavailable values are `null`.

Accepted temperature/humidity/code/hourly count and receipt age come from the runtime weather snapshot. `attempt.currentGeneration` distinguishes older attempt evidence from current source settings. Running attempts report running; terminal phase/error details appear on completion. No URL, query, redirect target, headers, response payload, credentials, or SDK log text is retained. Transport deadlines, certificate checks, response caps, and scheduling are unchanged. Diagnostics support investigation but do not independently establish why a forecast is absent.
