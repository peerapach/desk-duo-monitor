# Desk Duo Software Endpoints Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Deliver locally tested Google Sheets stocks endpoint source and a CodexBar app hook sender for the future ESP32 receiver.

**Architecture:** Apps Script reads a configured private sheet and returns bounded stocks JSON. A Python standard-library sender receives CodexBar hook JSON on stdin and posts normalized Claude/Codex usage to the ESP32 over LAN. ESP32 firmware and real cloud/account deployment are subsequent work after the required hardware and account facts are available.

**Tech Stack:** Google Apps Script V8; Node.js 20+ built-in test runner for Apps Script fixtures; Python 3.10+ standard library and unittest.

**Spec:** `docs/superpowers/specs/2026-10-04-desk-duo-design.md`

## Global Constraints

- Mac sends only Claude/Codex usage; weather and stocks are fetched by ESP32.
- No CodexBar CLI requirement; hook commands run directly without a shell.
- Two animated characters, portrait split layout, no session working/idle tracking in this version. This milestone defines data feeds, not display firmware.
- Stocks: `schemaVersion: 1`, generatedAt is response generation time, optional quotedAt is source quote time; missing numbers are not zero.
- Apps Script reads at most 200 data rows and returns at most 5 items; unknown columns are ignored. Sheet selection and credentials come from Script Properties.
- Authenticate Apps Script query `accessToken` before reading the Sheet; errors are coarse JSON codes without private IDs, tokens or upstream error text.
- Usage: input fractions 0–1 become usedPercent 0–100. Missing primary/secondary windows are omitted. No account identity forwarded.
- Sender requires a device token, uses HTTP POST /api/usage, refuses redirects, and bounds request duration; no secrets or full URLs in error messages.
- Shell commands start with `rtk`; use `rtk proxy` for unfiltered commands.
- Keep all changes and commits within project-local Git; no deployment, hook enablement, device flash, or edits to parent workspace.

## Review Focus

1. Blank formula cells and numeric zero: omitted missing values remain distinct from measured zero (Task 1).
2. Malformed/duplicated headers and invalid timestamps/numbers: do not return believable invented prices (Task 1).
3. Bad or missing accessToken: never open the sheet, return only auth_required (Task 1).
4. Missing secondary quota and invalid bool/NaN input: omit absent windows, reject invalid scalars before sending (Task 2).
5. Redirect/network failure/receiver errors: keep bearer token off other hosts and sanitize errors (Task 2).

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

This plan ships the two hardware-independent components. Hardware identity/pins, compiler installation, receiver firmware, portrait UI on physical device, Wi-Fi web configuration and live Sheet deployment are not claimed complete by these tasks. Continue with a firmware plan once the board facts are established. No protocol here requires weather or stock fetching on Mac.
