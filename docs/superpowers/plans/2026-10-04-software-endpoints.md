# Desk Duo Software Endpoints Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Deliver locally tested Sheets endpoint, CodexBar sender, and buildable CYD portrait firmware for the approved Desk Duo design.

**Architecture:** Apps Script returns bounded stocks JSON and the Python sender forwards normalized usage. Firmware receives usage, fetches configurable weather/stocks endpoints independently, and renders two animated characters on portrait CYD. User confirmed the hardware during execution; the two firmware tasks below extend the same plan and ledger, while upload and live cloud/account verification remain deferred.

**Tech Stack:** Google Apps Script V8; Node.js 20+ built-in test runner for Apps Script fixtures; Python 3.10+ standard library and unittest.

Firmware extension: C++17, cJSON from pinned ESP-IDF 5.5.3, LVGL 9.2.2; PlatformIO Core 6.2.0 / espressif32 6.13.0. Native compiler clang/clang++ for pure firmware-core tests. Public truststore 0.10.4 for build commands using existing system CAs.

**Spec:** `docs/superpowers/specs/2026-10-04-desk-duo-design.md`

## Global Constraints

- Mac sends only Claude/Codex usage; weather and stocks are fetched by ESP32.
- No CodexBar CLI requirement; hook commands run directly without a shell.
- Two animated characters, portrait split layout, no session working/idle tracking in this version. Tasks1–2 define data feeds; Tasks3–4 implement display firmware source and build.
- Stocks: `schemaVersion: 1`, generatedAt is response generation time, optional quotedAt is source quote time; missing numbers are not zero.
- Apps Script reads at most 200 data rows and returns at most 5 items; unknown columns are ignored. Sheet selection and credentials come from Script Properties.
- Authenticate Apps Script query `accessToken` before reading the Sheet; errors are coarse JSON codes without private IDs, tokens or upstream error text.
- Usage: input fractions 0–1 become usedPercent 0–100. Missing primary/secondary windows are omitted. No account identity forwarded.
- Sender requires a device token, uses HTTP POST /api/usage, refuses redirects, and bounds request duration; no secrets or full URLs in error messages.
- Shell commands start with `rtk`; use `rtk proxy` for unfiltered commands.
- Keep all changes and commits within project-local Git; no deployment, hook enablement, device flash, or edits to parent workspace.
- Firmware extension: user-confirmed ESP32-2432S028R, ESP32-D0WD-V3 rev3.1, 4MB, no PSRAM; portrait 240×320. Device is disconnected; build/native verification only.
- Preserve measured LCD/touch bus separation and upstream license notices; portrait MADCTL0x48 and corner mapping still require physical smoke tests.
- Config AP uses a generated per-device password and HTTP Basic admin auth; usage bearer token is separate. Stored secrets never appear in GET settings or logs.
- Display never waits on network IO; model and config updates are atomic, stale/error responses retain data only for the same configured source. Changing a source resets it to unknown and invalidates in-flight old responses.
- HTTPS uses verified certificate bundle and SNTP; at most4 redirects; cross-origin redirects drop configured headers; HTTPS→HTTP downgrade is rejected.
- Source bodies ≤16384 bytes, config body ≤8192, usage body ≤4096; parsing nesting depth ≤16; source fetch budget10 seconds. No full-screen multiple RGB565 buffers.

## Review Focus

1. Blank formula cells and numeric zero: omitted missing values remain distinct from measured zero (Task 1).
2. Malformed/duplicated headers and invalid timestamps/numbers: do not return believable invented prices (Task 1).
3. Bad or missing accessToken: never open the sheet, return only auth_required (Task 1).
4. Missing secondary quota and invalid bool/NaN input: omit absent windows, reject invalid scalars before sending (Task 2).
5. Redirect/network failure/receiver errors: keep bearer token off other hosts and sanitize errors (Task 2).

Firmware extension coverage: version/type/numeric validity, unknown versus zero, config-generation stale responses, source changes, redacted settings, mapped hourly timestamps, redirect header policy and data retention (Task 3). Target compile plus nonblocking task architecture, verified TLS, saved settings and physical test limitations (Task 4).

## Planning Discovery

Status: `READY — hardware-independent software milestone`.

| Surface | Evidence | Preserved contract / planned change | Verification | Boundary |
|---|---|---|---|---|
| Workspace | Only approved spec existed; project-local Git was initialized on codex/desk-duo after parent repository discovery | Scope all changes to esp32-monitor | Check git root and diff | No existing software to migrate |
| Stocks | Spec's columns, JSON, missing-value rule; Google ContentService and Web App docs | New endpoint source against spec addendum | Node VM handler tests; sample response | No real Sheet or deployment yet |
| Usage | Spec + primary receiver contract | New stdin normalizer and HTTP sender | Python unit tests and loopback HTTP receiver | Firmware contract only; not physical ESP32 verification |
| Runtime | Python available; bundled Node v24.19.0 at `/Users/peerapach.v/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node` | No third-party packages needed for this milestone | Native test runners | No PlatformIO / ESP-IDF and no detected USB board at preflight |

## Files and Ownership

- `integrations/google-sheets/Code.gs`: pure table-to-payload function and authenticated Apps Script handler.
- `integrations/google-sheets/README.md`: Thai setup/deploy guide and price-source boundaries.
- `tests/stocks_endpoint.test.cjs`: Node VM tests with only Google service boundaries simulated.
- `bridge/codexbar_sender.py`: stdin normalization, validated config and bounded HTTP POST.
- `tests/test_codexbar_sender.py`: Python normalization/transport tests, including actual loopback HTTP integration.
- `examples/stocks-response.json`, `examples/usage-event.json`, `examples/usage-response.json`: non-secret fixtures.
- `examples/bridge-config.example.json`, `examples/codexbar-hooks.example.json`: configuration templates with dummy values.
- `README.md`: Thai project entry point, run checks, known verification limits and next hardware step.
- `docs/protocol.md`: precise stocks/usage receiver contract for firmware implementers.

### Task 1: Google Sheets stocks endpoint

**Files:** Create `integrations/google-sheets/Code.gs`, `integrations/google-sheets/README.md`, `tests/stocks_endpoint.test.cjs`, `examples/stocks-response.json`.

**Interfaces:**
- Consumes Script Properties SPREADSHEET_ID, SHEET_NAME, ACCESS_TOKEN and doGet(e).parameter.accessToken.
- Produces `buildStocksPayload(rows, now)` (pure, table including header row, JS Date) and `doGet(e)` (ContentService TextOutput).
- Success: schemaVersion 1, generatedAt UTC ISO, items (max 5), each symbol, label, currency, optional price/changePercent/quotedAt.
- Failure: schemaVersion 1, error.code: auth_required, configuration_error, invalid_sheet, source_unavailable.
- Exposes no Node-specific require/module syntax in Code.gs; Node tests load it through vm and mock Google boundary services.

- [ ] **Step 1: Write tests for the public behavior before implementation.** Representative assertions:

```javascript
const rows = [['symbol','price','changePercent','quotedAt'], ['PTT', 0, '', new Date('2026-10-04T03:00:00Z')]];
assert.deepEqual(build(rows, now).items, [{symbol:'PTT', label:'PTT', price:0, currency:'THB', quotedAt:'2026-10-04T03:00:00.000Z'}]);
assert.equal(build(rows, now).generatedAt, now.toISOString());
assert.equal(callHandler({parameter:{}}, services).error.code, 'auth_required');
assert.equal(services.openCount, 0);
```

Also cover: reordered headers, trailing blank rows, empty sheet/header-only, missing symbol/price header, duplicate recognized header, nonfinite/boolean/nonnumeric cells, ISO date with explicit offset normalized to UTC, invalid or ambiguous dates, defaults for blank label/currency, wrong token, missing properties, missing tab, upstream exception redaction, numeric formula results from getValues(), max 5 symbols in sheet order, reading cap, optional fields omitted, unknown columns ignored.

- [ ] **Step 2: Run RED.**

Run `rtk proxy /Users/peerapach.v/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node --test tests/stocks_endpoint.test.cjs`. Record relevant failing output and why the missing behavior causes it.

- [ ] **Step 3: Implement buildStocksPayload and doGet.**

Authenticate before any SpreadsheetApp call. Use openById(property), getSheetByName(property), and bounded getRange(1,1,Math.min(lastRow,201),lastColumn).getValues(), not full getDataRange on arbitrary sheets. No token accepted via header because Apps Script event does not expose arbitrary request headers. Header matching trims strings and is case-insensitive; duplicate recognized header and missing required header return invalid_sheet. Ignore rows with blank symbol. Preserve zero and omit blanks. Accept numeric cells or trimmed unambiguous decimal/exponent numeric strings only; reject nonfinite, booleans, locale-ambiguous strings. Nonnegative price; signed changePercent. Preserve row order and stop after 5 included rows. QuotedAt accepts valid Date objects or valid ISO datetime strings with timezone; omit blank values, reject ambiguous/invalid dates. Limit symbol/label/currency lengths to 32/64/8 characters; no embedded control characters. Use pure validation and stable error codes; ContentService application/json response only, no raw exceptions.

- [ ] **Step 4: Write the fixture and Thai setup guide.**

Document required headers and defaults, script properties, generating a fresh random token locally, private Sheet + web app execute-as-owner, configurable access settings subject to Workspace policy, token query URL, redirect handling, JSON errors normally arriving with HTTP 200, max 5 entries, optional price source timestamps vs generatedAt, token rotation, and the fact that stocks source automation is not built. Include sample rows and actual Code.gs installation instructions; do not deploy anything. Never request the user paste real tokens into chat.

- [ ] **Step 5: Run GREEN, self-review and commit.**

Run the Node test command above; expect all cases pass with no warnings. JSON-parse the example fixture. Commit only task-owned files using `rtk git add ...` then `rtk git commit -m "feat: add authenticated Sheets stocks endpoint"`. Include RED/GREEN commands and output in the report.

### Task 2: CodexBar sender and project integration

**Files:** Create `bridge/codexbar_sender.py`, `tests/test_codexbar_sender.py`, `examples/usage-event.json`, `examples/usage-response.json`, `examples/bridge-config.example.json`, `examples/codexbar-hooks.example.json`, `docs/protocol.md`, `README.md`.

**Interfaces:**
- Consumes stdin JSON usage_updated event and `--config FILE` JSON {endpoint, bearerToken, timeoutSeconds}; --dry-run emits normalized usage JSON without networking.
- Produces `normalize_event(event: dict) -> dict`, `load_config(path: Path) -> dict`, `send_usage(payload: dict, config: dict) -> None`, `main(argv=None) -> int`.
- Payload: {schemaVersion:1,type:'usage',provider:'claude'|'codex',updatedAt:UTC ISO,primary?:{usedPercent,windowMinutes?,resetsAt?},secondary?:same}. Optional windows require corresponding usage fraction; zero is present data.
- Receiver: POST /api/usage; Content-Type application/json, Authorization Bearer. Any HTTP 2xx counts as delivered. Real device must implement this contract in subsequent firmware work.
- Uses only Python standard library; tests import without invoking main or network.

- [ ] **Step 1: Write failing tests for normalization, config and actual delivery.** Representative assertions:

```python
payload = normalize_event({"event":"usage_updated", "provider":"codex", "timestamp":"2026-10-04T10:00:00+07:00", "usagePercent":0, "secondaryUsagePercent":0.5})
self.assertEqual(payload["updatedAt"], "2026-10-04T03:00:00Z")
self.assertEqual(payload["primary"]["usedPercent"], 0)
self.assertEqual(payload["secondary"]["usedPercent"], 50)
self.assertNotIn("account", payload)
```

Cover event/provider/type validation; negative/>1 fractions; bool/nonfinite numbers; unknown fields and identities ignored; omitted secondary and omitted optional fields; timestamp offset normalization with explicit timezone required; malformed datetime; positive finite windowMinutes; both windows absent preserved as no windows (unknown data, not zero); input >4096 bytes rejected; invalid JSON/nonobject; config nonempty token/URL/finite timeout 0.1..30 default 5 seconds; token control characters rejected; endpoints http/https with /api/usage path and no credentials/query/fragment; dry-run succeeds without config and never networks. Integration fixture asserts actual POST path/body/headers; returns 204 for success, 500 for receiver rejection, 302 for redirection; the redirect destination must receive zero requests. Timeout and failures return sanitized errors without token or endpoint.

- [ ] **Step 2: Run RED.**

Run `rtk proxy python3 -m unittest discover -s tests -p 'test_*.py' -v`. Record relevant failure evidence. Loopback tests may need the tool's approved local-network sandbox escalation; no real endpoints are called.

- [ ] **Step 3: Implement sender.**

Read at most 4097 bytes from stdin and reject >4096. Normalize explicitly with allowlisted fields, validate all finite numeric scalars and timezone-aware dates before HTTP. Omit window details if usage percentage is missing; do not invent quotas. Require config only outside --dry-run. Validate URL/token/timeout before constructing requests. Use urllib Request and an opener whose redirect handler rejects all redirects before bearer headers can reach another host. Bound timeout, disable accidental response-body logging, treat 2xx as success and reject other statuses. CLI exits 0 on success, 2 for invalid input/config, 1 on delivery failure, printing only coarse error messages to stderr. No automatic retries and no polling loop; CodexBar controls hook cadence.

- [ ] **Step 4: Document protocols and wire examples.**

Thai README links the Sheets guide, explains source-only status, provides --dry-run command, manual posting test, private `bridge.local.json` permission guidance, and direct CodexBar app hook config (absolute Python executable + absolute sender script + absolute config, each provider an enabled usage_updated rule). The sample config must be an explicit merge fragment, never instructions to overwrite provider/secret config. Dummy token/paths clearly marked; no live config changes. Document current hook's 600-second cap as version-dependent, no CodexBar CLI requirement, receiver still requiring firmware, unknown hardware model, weather independent on ESP32 and Open-Meteo config planned after board confirmation. Document payload field units, examples and source timestamps.

- [ ] **Step 5: Run both suites, self-review and commit.**

Run Python unittest command and Node endpoint test command. Expect all pass. Run dry-run from the event fixture and compare parsed output with usage-response.json; parse all example JSON. Run `rtk git diff --check`. Commit only task-owned files with `rtk git commit -m "feat: forward CodexBar usage to device receiver"`. Full report includes commands/results and local-only verification limits.

## Execution Boundary

Tasks1–2 ship the hardware-independent components. Hardware is now confirmed, so Tasks3–4 add firmware source and target compilation to this same execution. No upload, successful backup or live Google/CodexBar/physical display result is claimed. No protocol requires weather or stocks fetching on Mac.

## Planning Discovery — firmware extension

Status: READY for source/build/native verification; user removed board due to low battery. Read-only /root/firmware_discovery established CYD pins, separate SPI touch, IDF verified-CA route, upstream memory constraints and license. Primary checked installed platform manifest: framework-espidf~3.50503.0; SDK cJSON source/header are present. PlatformIO6.2.0 and SDK installed project-locally. Private CA download issue resolved by existing macOS trust; no certificate validation disabled. Evidence and source links are in the spec firmware addendum and this plan's workspace firmware-discovery.md.

### Task 3: Portable firmware model, parsers and settings policy

**Files:** Create `firmware/main/core.hpp`, `firmware/main/core.cpp`, `tests/firmware_core_test.cpp`, `scripts/test_firmware.py`, `scripts/pio.py`, `scripts/build_trust/sitecustomize.py`, `requirements-build.txt`, `examples/device-config.example.json`, `examples/weather-response.json`.

**Interfaces:**
- Namespace `deskduo`; no GPIO, FreeRTOS, Wi-Fi or LVGL imports in core files. Use C++17 and SDK cJSON for parsing.
- `template<typename T> struct ParseResult` contains `std::optional<T> value` and `std::string error` (empty on success, coarse code on failure).
- Public types `Usage`, `Stocks`, `Weather`, `SourceConfig`, `Config`, `Model`, `Snapshot`, `TouchCalibration`; define all in core.hpp. Use bounded strings/containers per spec; epochs int64 UTC seconds, source-arrival time uint64 monotonic milliseconds.
- `parseUsage(const std::string&) -> ParseResult<Usage>`, `parseStocks(const std::string&) -> ParseResult<Stocks>`, `parseWeather(const std::string&, const SourceConfig&) -> ParseResult<Weather>`.
- `defaultConfig() -> Config`, `applyConfigPatch(const Config&, const std::string&) -> ParseResult<Config>`, `parseStoredConfig(const std::string&) -> ParseResult<Config>` (validated internal NVS restoration, including generated setupPassword), `serializeConfig(const Config&) -> std::string` (internal NVS only), `redactedConfig(const Config&) -> std::string` (public authenticated settings).
- Config JSON fields: ssid, wifiPassword, usageToken, setupPassword (internal only); weather/stocks each {url,headers object,refreshSeconds}; weather additionally {label,fields object}; autoRotateSeconds; touch {swapAxes,invertX,invertY,minX,maxX,minY,maxY}. SetupPassword must not be writable through config patch.
- Weather fields names: temperature, humidity, weatherCode, updatedAt, utcOffsetSeconds, hourlyTimes, hourlyTemperatures, hourlyRainProbability. Default mappings for Open-Meteo current.temperature_2m / current.relative_humidity_2m / current.weather_code / current.time / utc_offset_seconds / hourly.time / hourly.temperature_2m / hourly.precipitation_probability. Dot paths may include bounded array indexes; reject malformed paths; max128 bytes.
- `Model::applyConfig(const Config&)`, `Model::acceptUsage(const Usage&, uint64_t nowMs)`, `Model::acceptStocks(const Stocks&, uint64_t nowMs, uint64_t generation)`, `Model::acceptWeather(const Weather&, uint64_t nowMs, uint64_t generation)`, `Model::markFailed(const std::string& source, uint64_t generation)`, `Model::snapshot(uint64_t nowMs) -> Snapshot`, `Model::generation() -> uint64_t`.
- Snapshot exposes only render data, receipt times, stale/unknown flags and nonsecret display settings. No config passwords/tokens/headers/URLs in render snapshot.
- `redirectHeadersAllowed(const std::string& from,const std::string& to) -> bool` true only for same origin; `redirectTargetAllowed(from,to) -> bool` rejects HTTPS downgrade and invalid schemes/userinfo/control characters. `mapTouch(int rawX,int rawY,const TouchCalibration&) -> std::pair<int,int>` clamps to portrait coordinates.

- [ ] **Step1: Write native failing tests for public behaviours.** Use real parsing/model functions and existing shared fixtures. Representative assertions:

```cpp
auto u = deskduo::parseUsage(usageFixture);
assert(u.value.has_value());
assert(u.value->primary->usedPercent == 32.0);
auto absent = deskduo::parseUsage(R"({"schemaVersion":1,"type":"usage","provider":"claude","updatedAt":"2026-10-04T03:00:00Z"})");
assert(absent.value && !absent.value->primary && !absent.value->secondary);
auto zero = deskduo::parseStocks(R"({"schemaVersion":1,"generatedAt":"2026-10-04T03:00:00Z","items":[{"symbol":"PTT","price":0,"currency":"THB"}]})");
assert(zero.value && zero.value->items[0].price == 0.0);
```

Required cases: invalid JSON/trailing data/nesting17; wrong schema/provider/types/bool/nonfinite/ranges; leap dates/timezone/fractional seconds; stocks missing values, invalid negative price, >5 items, error envelope; weather zero/negative temperature, humidity0..100, invalid WMO code, optional missing arrays,4 forecast slots, local-naive timestamps plus utc_offset_seconds25200 vs no-offset unknown; path lookup indexes/malformed path. Config patch preserves omitted fields; rejects oversized URL/headers, CRLF/userinfo, invalid intervals, rotation1, unauthorized setupPassword change, invalid touch spans; round-trip internal config and public redaction must omit all secrets and URL queries. Model first failure stays unknown, later failure retains values/stale, stale age threshold2×refresh, old generation ignored, changed source clears old data, older usage does not overwrite newer same provider, both providers independent. Redirect same-origin/default-port handling and HTTPS downgrade; touch clamps all four endpoints into240×320. Tests assert actual outputs, not implementation structure.

- [ ] **Step2: Run RED.** `rtk proxy python3 scripts/test_firmware.py` builds test binary with clang/clang++, core sources, and SDK cJSON.c. If runner absent initially compile equivalent manually; record assertion failures caused by missing behaviour. The runner locates SDK from PLATFORMIO_CORE_DIR or project .platformio, uses temp/output under project .platformio/native-tests, compiles cJSON as C, then core/tests as C++17; does not need device or network.

- [ ] **Step3: Implement core parsers/settings/model.** All validation before state changes; JSON max bounds and nesting cap16 including strings/escapes. Whitespace after root allowed, trailing payload rejected. Epoch conversion independent of process timezone, calendar validated, no unchecked unsigned conversion. Unknown optional values remain optional; state failure doesn't fabricate zeros. Accept stocks source fields per committed endpoint; configuration-source changes clear only corresponding cached data and invalidate in-flight generation. Source-generation acceptance is explicit. Snapshot uses monotonic age, no secrets. Config defaults have endpoints empty, refresh900/300, rotation20, label Weather, empty credentials for runtime RNG provisioning. Usage token if provided length16..128 printable non-whitespace; WiFi SSID≤32 bytes/password≤64; setup password generated runtime and never patched. Header bounds6 /64/256, URL1024, mapping128, body8192; touch defaults based on raw12-bit ADC with transform configurable, not claimed calibrated. Keep source header/private query values only in serialized internal config, never redacted view.

- [ ] **Step4: Add build/native helpers and fixtures.** requirements-build pins platformio6.2.0 + truststore0.10.4. `scripts/pio.py` runs PlatformIO as a CLI using public truststore.inject_into_ssl (application-only) and project .platformio; propagate existing system trust into build Python subprocesses via an explicitly scoped sitecustomize bootstrap (no global Python/settings edits and no disabling SSL). Preserve existing PYTHONPATH, derive trusted installed package location rather than arbitrary downloaded CA. It only acts for this build command; does not modify Python sender runtime. Native runner/fixtures include no real credentials. Example device config uses dummy secrets/empty URLs and fields above; weather fixture values explicitly simulated. No SDK vendoring into Git.

- [ ] **Step5: Run native GREEN and current full suites, self-review, commit.** Native tests via runner; Python unittest and Node endpoint tests via existing commands. Parse examples and diff check. No device or live endpoints. Commit only task-owned files. Report TDD, compiler commands, tests and public header contract for Task4.

### Task 4: CYD runtime, portrait UI, source networking and configuration portal

**Files:** Create `firmware/platformio.ini`, `firmware/CMakeLists.txt`, `firmware/sdkconfig.defaults`, `firmware/partitions.csv`, `firmware/main/CMakeLists.txt`, `firmware/main/main.cpp`, `firmware/main/runtime.hpp`, `firmware/main/runtime.cpp`, `firmware/main/display.hpp`, `firmware/main/display.cpp`, `firmware/main/input.hpp`, `firmware/main/input.cpp`, `firmware/main/network.hpp`, `firmware/main/network.cpp`, `firmware/main/portal.hpp`, `firmware/main/portal.cpp`, `firmware/main/ui.hpp`, `firmware/main/ui.cpp`, `firmware/main/assets.*`, `firmware/main/fonts/*` as needed, `firmware/main/idf_component.yml`, `firmware/dependencies.lock` from build, `firmware/README.md`, `THIRD_PARTY_NOTICES.md`, `scripts/package_firmware.py`. Update `README.md`, `docs/protocol.md`, `.gitignore` for managed/generated dependencies and artifacts; update native tests only to cover new pure policy helpers if required.

**Interfaces:**
- Read core.hpp and Task3 report, use exact implemented core contracts, not parallel JSON/model implementation.
- `app_main` initializes NVS, per-device secrets, LVGL/display/input, WiFi AP+STA, portal and network task. Runtime owns Model/Config and short-held FreeRTOS mutex; helpers provide validated config/model snapshots; no secrets in render state.
- `display_init()` / `display_flush(x1,y1,x2,y2,pixels)` bind measured LCD SPI2; `input_init()` / `input_read(...)` uses SPI3, core touch mapping. `ui_init(...)` / `ui_tick(...)` operate on LVGL in one UI task only. `network_task(void*)` copies config under lock then releases it during IO; update only if generation unchanged.
- Portal GET `/` and `/api/config` authenticated HTTP Basic (`admin`, setupPassword). POST `/api/config` requires Basic + application/json and validated patch; save NVS before atomic apply. POST `/api/usage` requires separate bearer + application/json; payload max4096, validate before model update; wrong token401, invalid payload400, success204. GET config uses redactedConfig only. Persist failures return503 without applying memory change.

- [ ] **Step1: Test required policy seams before implementation.** Add genuine native tests for any introduced helpers: auth role separation and constant-time comparison if used, body-length/content-type rejection, config generation on changed source, redirect count4 and header stripping on cross-origin, safe source errors/redacted settings. Existing portable cases cover core behaviour; actual GPIO/RTOS/SDK calls require target build and future hardware tests, not mirror tests against source text.

- [ ] **Step2: Implement target build and runtime.** Pin espressif32@6.13.0 / framework espidf / board esp32dev / src_dir main; LVGL9.2.2 via component manager. Enable certificate bundle, C++17, required LVGL fonts and partial software renderer; no PSRAM. Factory partition fits4MB, no OTA feature required. Random12-char WPA AP password stored NVS/displayed setup; independent random32-hex usage token. SSID DeskDuo-<short device suffix>; AP address192.168.4.1. Existing config survives reset; validated save/application has one owner, config generations prevent stale response races. Basic admin applies only portal; usage token cannot modify settings. Protect writes against cross-origin simple-form requests by requiring JSON; do not add permissive CORS. Wrong auth never leaks settings. Secrets never logged or echoed by GET; credential inputs blank unless user edits.

- [ ] **Step3: Implement source networking and time.** Direct device GET weather/stocks, one source at a time in network task. Use verified IDF TLS bundle/hostname and SNTP before HTTPS. Honor 10-second overall budget and max16384 response; never rely solely on socket inactivity for total deadline. Follow ≤4 redirects with verified new target, reject HTTPS downgrade, drop all configured headers on origin change. Keep URL queries/credentials out of logs. Don't hold model/config lock during IO; invalid source data only marks source failed, no overwrite. URL-empty sources issue no request. Source change discards old response via generation. Request failure/timeout does not stop animation or other polling indefinitely. Record SDK API/bounds evidence and any off-board verification limits.

- [ ] **Step4: Implement selected portrait UI and setup portal.** Match the approved split mockup: Claude upper/Codex lower with their own primary/secondary bars and resets; both loop animations independent of real agent state. Weather page current conditions+four forecast slots; Stocks shows3 rows at a time and cycles remaining entries when >3; mini Claude+Codex in both pages. Show age/stale/unknown honestly; no fake startup numbers/clock. Clock default Bangkok timezone for this user, SNTP failure shows unknown time. Use LVGL partial buffer240×24×2 only; efficient sprite/geometry assets, no hundreds of per-pixel heap widgets or multiple full-screen buffers. Use Thai-compatible font(s) for Thai screen labels; preserve upstream/font notices when copied. First setup screen shows AP credentials, portal address and device usage token for local configuration; a BOOT-button action can show setup again without erasing config. Portal forms set WiFi/endpoints/headers/mappings/intervals/rotation/token with validated patch; no prefilled stored secrets. Touch driver on separate SPI3, swipe changes pages; portrait transform is configurable and physical corner check deferred.

- [ ] **Step5: Verify target build and package without flashing.** Run `rtk proxy .venv/bin/python scripts/pio.py run -d firmware -e cyd` with allowed download escalation if needed. Expect SUCCESS, report RAM/flash usage. Run complete native+Python+Node tests on final tree and JSON example parse/diff check. `scripts/package_firmware.py` merges actual built bootloader/partitions/application offsets using installed esptool, creates ignored `artifacts/desk-duo-cyd.bin` plus SHA256/metadata; no serial options or device writes. Verify merged image metadata/size. Thai firmware guide covers setup, bridge/IP/token, Sheets endpoint/redirect, weather config, backup retry at conservative baud115200, future image upload command and orientation/colors/touch/heap/slow-network smoke checklist. Update root README so it no longer says receiver not implemented, while stating physical/live/deployment verification still pending. Commit task-owned source only, not caches/font build intermediates/binary/private config. Final report includes full commands/output and no physical success claim.
