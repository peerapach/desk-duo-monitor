# Pocket Studio and Local OTA Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Deliver the approved black portrait Pocket Studio UI with recognizable animated Claude/Codex artwork and authenticated Wi-Fi application updates that preserve settings.

**Architecture:** Keep the existing runtime, JSON endpoints, and Mac hook bridge. Compile the approved raster mascots into bounded flash assets used by actual LVGL UI and its native preview. Add a local authenticated streaming upload to the inactive OTA slot, boot validation/rollback, and a settings-preserving USB migration path.

**Tech Stack:** ESP-IDF 5.5.3, LVGL 9.2.2, PlatformIO espressif32 6.13.0, C++17, Python stdlib packaging, Node/sharp asset compiler, actual-LVGL native preview.

**Spec:** `docs/superpowers/specs/2026-10-04-pocket-studio-ota-design.md`

## Global Constraints

- Portrait 240×320 CYD ESP32-2432S028R, pure black background, approachable and polished.
- Keep Claude above Codex, with clear usage and reset times. Preserve canonical Claude's connected stepped rectangle, two black square eyes, four short rectangular legs, and short side protrusions.
- Real bottom navigation for Duo, Weather, Stocks, Setup; visible LAN IP.
- Mac runs CodexBar hooks and sends only Claude/Codex usage directly to ESP32. Keep the bridge and `/api/usage` contract unchanged.
- No Outlook calendar or appointment integration; the user explicitly declined it.
- Pi data/image service and a live Windy feed are outside this release. Never display illustrative map art as live weather data.
- NVS remains at `0x9000`, size `0x16000`; PHY remains at `0x1f000`, size `0x1000`. OTA data at `0x20000`, size `0x2000`; OTA 0 at `0x30000`, OTA 1 at `0x210000`, each size `0x1e0000`.
- Wi-Fi, admin credential, usage token, endpoint settings, and touch calibration must survive migration and future OTA. No full merged-image write to an already-configured board.
- The main task remains the sole LVGL owner. Do not introduce PSRAM dependence or whole-firmware RAM buffering.
- Updates require existing Basic admin authentication; a usage bearer token does not authorize updates. Reject cross-origin browser uploads and require an explicit upload header without permissive CORS.
- Abort interrupted/invalid uploads without switching boot selection. Enable rollback; accept a trial image only after local initialization succeeds. Internet access and station association are not health requirements.
- No fuse changes, credential replacement, cloud exposure, or remote automatic updates in this scope.
- Prefix shell commands with `rtk`. Read workspace instructions. Implementers do not spawn subagents; controller owns review.

## Planning Discovery

Status: `READY`

| Surface | Evidence | Change / preserved contract | Verification | Risk |
| --- | --- | --- | --- | --- |
| Partition/migration | `firmware/partitions.csv`, `scripts/package_firmware.py`, current package metadata | Replace factory layout; retain NVS/PHY ranges; provide segmented USB migration | Generated partition geometry, image capacity, migration segment bounds, device settings compare | Current full-flash package erases NVS and old app offset is invalid for new layout |
| HTTP update | `portal.cpp:route`, `portal.hpp:authorized`, `assets.cpp:portal_html` | New binary route with Basic auth and streaming; preserve JSON routes | Authorization, media type/length/origin, timeout, invalid image, interruption | Existing JSON route buffers entire body; HTTP server has three sockets |
| Boot/recovery | `main.cpp:app_main`, `runtime.cpp:runtime_init/runtime_patch`, SDK config | Enable effective rollback config; accept after local health checks; preserve NVS format | Build flags, boot state tests, hardware update and rollback | Rollback does not restore changed NVS |
| UI/art | `ui.cpp`, `assets.cpp`, actual-LVGL native preview | Black four-tab UI; compiled approved art; keep Thai shaping and explicit local credential reveal | Real 240×320 renders, navigation, unknown/stale/long strings, credential close/reopen | Old tests use child indexes and BOOT-only Setup |
| Packaging/integration | `package_firmware.py`, bridge and existing tests | Validate OTA slots and all segments; update instructions and metadata | Core/Python/Node/native/target/package checks | Final asset/code size must fit one slot |

Ownership: runtime owns NVS and settings writer synchronization; portal owns HTTP; network owns APSTA and source polling; main owns LVGL. Build identity is `desk_duo`. Existing application size is 1,429,360 bytes; each new slot is 1,966,080 bytes. Do not change persisted config format. Current source art is `firmware/assets/pocket-studio-characters.png` (1774×887 RGBA, Claude left/Codex right).

## File Structure

- `scripts/compile_character_assets.mjs`: reproducible source-PNG to small RGB565 flash assets. Generated include belongs under `firmware/main/generated/`.
- `firmware/main/character_assets.hpp/.cpp`: LVGL image descriptors and character frame selection, shared by native and target builds.
- `firmware/main/ui.cpp`: layout, navigation, actual data/unknown/stale states and decorative animation.
- `firmware/main/ota_policy.hpp`: portable upload policy/image identity checks used by production and native tests.
- `firmware/main/ota.hpp/.cpp`: streaming ESP-IDF update transaction, synchronized maintenance state, status, boot confirmation.
- `portal.cpp`, `assets.cpp`, `main.cpp`, `network.cpp`: OTA route/browser upload/startup/polling integration, preserving existing contracts.
- `scripts/package_firmware.py`, `scripts/migrate_ota.py`: verified packages and explicit segmented USB migration; default migration is dry-run.
- Native preview and focused OTA/package/portal tests exercise production paths. Docs describe current usable behavior and distinguish hardware checks still pending.

### Task 1: Approved Pocket Studio UI and real character artwork

**Files:**
- Create: `scripts/compile_character_assets.mjs`, `firmware/main/character_assets.hpp`, `firmware/main/character_assets.cpp`, `firmware/main/generated/character_pixels.inc`.
- Add prepared source: `firmware/assets/pocket-studio-characters.png`, `firmware/assets/README.md`.
- Modify: `firmware/main/ui.cpp`, `firmware/main/CMakeLists.txt`, `firmware/sdkconfig.defaults`, effective `firmware/sdkconfig.cyd` if tracked/configured, `tests/native_ui/CMakeLists.txt`, `tests/native_ui/lv_conf.h`, `tests/native_ui/preview.cpp`, relevant UI documentation.
- Preserve `assets.cpp` portal HTML for Task 2; obsolete ASCII art may be removed if no caller remains.

**Interfaces:**
- Consumes existing `runtime_snapshot()`, `runtime_config()`, `station_address()`, `ap_name()`, Thai `shapeText()`, LVGL and GPIO BOOT.
- Produces four working UI tabs and shared compiled character descriptors. Suggested public boundary, define consistently:

```cpp
// character_assets.hpp
#pragma once
#include "lvgl.h"
namespace deskduo {
const lv_image_dsc_t* character_image(unsigned provider, bool mini = false);
}
```

- No OTA-specific UI calls in this task; Task 2 provides upload progress through the browser. Keep UI responsive under concurrent maintenance.

- [ ] **Step 1: Add meaningful failing native navigation checks.** Locate buttons through labels/events rather than private child indexes. Assert Setup can be reached by its bottom button, the IP is present on each normal page, and tokens are absent after leaving Setup. Preserve the long-token close/reopen/reflow checks. Use the real UI build.

```cpp
auto* setupLabel = findLabel(lv_screen_active(), deskduo::shapeText("ตั้งค่า").c_str());
assert(setupLabel);
lv_obj_send_event(lv_obj_get_parent(setupLabel), LV_EVENT_CLICKED, nullptr);
deskduo::ui_tick();
assert(findLabel(lv_screen_active(), "192.0.2.10"));
```

- [ ] **Step 2: Run the new checks before implementation and retain the expected failure evidence.** Configure with `rtk proxy .venv/bin/cmake -S tests/native_ui -B .platformio/native-ui -DCMAKE_BUILD_TYPE=Debug`; build and ctest with the same CMake binary. If that path is unavailable, use the installed CMake documented in `firmware/README.md` and record it.

- [ ] **Step 3: Compile the approved artwork and implement the black layout.** Use bundled Node/sharp (Node executable `node`; sharp module at its `dependencies/node/node_modules/sharp`). Accept an override dependency path for portability. Split the source at its midpoint, find each character's nontransparent bounds, preserve aspect ratio, and compile 64×64 main / 32×32 mini RGB565 images against black. Commit generated bytes and source recipe; firmware must not decode the full source PNG. This is an asset format conversion, not a new art direction. Main image count is bounded; movement can be native position/scale animation without duplicating many frames. Inspect the converted shapes at target size.

```js
const { data, info } = await sharp(source).ensureAlpha().raw().toBuffer({resolveWithObject:true});
// Scan each half's alpha bounds before resizing its isolated region.
const rgb565 = (r, g, b) => ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
```

Layout: header 24px; content y=24 height=232; IP y=256 height=20; four 60×44 touch buttons y=276. Two provider rows fit within content, with art left, native provider/value/reset text right, fine divider and thin progress bars. Use black `0x000000`, near-white `0xF0F1EA`, readable gray `0xA3AAA8`, copper `0xEBA779`, mint `0xC6E7DC`. Enable matching Montserrat 32 on target/native if used for primary numbers. Preserve both independent windows and their reset dates; format arbitrary window minutes correctly rather than hardcoding 5h/7d. Missing windows show `--`; stale/unknown remains distinguishable. Stock values/currency/quote timestamps retain actual source semantics and paging through up to five entries. Weather shows real numeric/condition/forecast data; no fabricated radar. Setup retains protected AP/admin details and usage token only in explicit local reveal; long token content scrolls inside content without covering navigation. Main page IP never looks like a button. Existing touch config is retained; do not flip calibration defaults again.

For animation use small deliberate movement (breathing/brief tap greeting), not constant jumping. Do not infer session activity from usage. Auto rotation skips Setup; preserve swipe and BOOT navigation as secondary controls, with sensible post-tap hold so the selected page does not immediately rotate away.

- [ ] **Step 4: Verify real rendering and data behavior.** Run native assertions; emit normal/unknown/stale/long reset/long token/stock-extreme renders. Run core checks and relevant portal tests once. Build target once after UI works; confirm assets are flash-resident and memory remains bounded. Do not create source images of fake data as firmware UI.

- [ ] **Step 5: Inspect your diff, document results, and commit only task files.** Include source art and conversion recipe. Report exact commands/output, actual renders, size, concerns. Controller performs separate task review.

### Task 2: Authenticated OTA, boot rollback, and safe USB migration

**Files:**
- Create: `firmware/main/ota_policy.hpp`, `firmware/main/ota.hpp`, `firmware/main/ota.cpp`, `scripts/migrate_ota.py`, focused OTA and packaging tests with a runner integrated into existing test commands.
- Modify: `firmware/partitions.csv`, `firmware/sdkconfig.defaults`, effective `firmware/sdkconfig.cyd`, `firmware/CMakeLists.txt`, `firmware/main/CMakeLists.txt`, `firmware/main/main.cpp`, `firmware/main/portal.cpp`, `firmware/main/assets.cpp`, `firmware/main/network.cpp`, `scripts/package_firmware.py`, portal tests, `firmware/README.md`, top-level docs as needed.

**Interfaces:**
- Consumes Task 1 UI with no LVGL calls from update workers; existing Basic admin auth; existing NVS and APSTA initialization.
- Produces binary `POST /api/firmware`, authenticated `GET /api/firmware` status/version/capacity, manual browser file upload, dual OTA slots, and a dry-run-first USB migration tool.
- Define consistent module boundary:

```cpp
// ota.hpp
#pragma once
#include <string>
#include "esp_http_server.h"
namespace deskduo {
bool ota_busy();
std::string ota_status_json();
esp_err_t ota_upload(httpd_req_t* request);
void ota_confirm_boot();
}
```

- Portable policy must be the actual production policy, not an unrelated mocked duplicate. Protect maintenance transitions atomically. `runtime_patch` NVS serialization remains unchanged; portal rejects settings mutations while OTA owns maintenance, and network polling/configuration must not reconnect the station during upload. Use bounded locking, never hold a lock across LVGL or long flash/network I/O.

- [ ] **Step 1: Write meaningful failing upload/migration tests.** Cases: usage bearer vs Basic admin, absent/wrong upload header, cross-origin Origin (including suffix/userinfo mistakes), allowed same-origin or authenticated non-browser request without Origin, wrong media type, zero/oversize length, wrong chip/project, malformed/truncated image, timeout/interruption, no boot switch on failure, single-upload admission, and settings busy state. Packaging tests parse actual-format partition fixtures and reject changed NVS/PHY, overlaps, out-of-flash or undersized slots, wrong app offset, missing OTA initialization, and merged packages passed as application updates. Migration tests assert write segments never touch NVS/PHY and a fresh private backup occurs before any write.

```cpp
// Policy cases use the same helpers invoked by ota_upload.
assert(upload_origin_allowed("http://192.0.2.10", "192.0.2.10"));
assert(!upload_origin_allowed("http://192.0.2.10.evil.invalid", "192.0.2.10"));
assert(!upload_origin_allowed("http://192.0.2.10@evil.invalid", "192.0.2.10"));
```

Choose helpers with explicit signatures and use those exact signatures in production/test; the above names are a required semantic seam, not a request for a duplicated test-only function.

- [ ] **Step 2: Run focused tests to establish RED.** Use cached SDK/core compiler and existing Python/Node runners. Record failures attributable to missing OTA behavior rather than environment/dependency setup.

- [ ] **Step 3: Implement streaming update and maintenance integration.** Require Basic admin before reading upload, `Content-Type: application/octet-stream`, `X-DeskDuo-Update: 1`, fixed positive `Content-Length` within inactive slot. Validate Origin when present against exact HTTP origin/Host; header requirement and absent permissive CORS prevent browser form uploads. Publish no credentials. Receive in chunks (e.g. 4096 bytes), with a bounded total transfer deadline (120 seconds) and idle timeout (10 seconds); tolerate bounded socket timeout retries until idle expiry. Verify ESP image magic/header/chip and `esp_app_desc_t.project_name == "desk_duo"` from the streamed prefix, then let `esp_ota_end` validate full image. Reject merged packages. Abort transaction on every failure and select the new boot slot only after all bytes and image validation succeed. This is not a signed publisher-identity guarantee; existing admin auth is the trust boundary.

Use ESP HTTP Server async request ownership / an update worker so the HTTP server can still serve authenticated status and usage. Bound to one worker/upload; correctly complete async request handles on each failure/success path. Send the success response before delayed reboot. Keep three-socket limitations in mind and verify no LRU eviction of the upload transaction. If socket count must increase, bound it and account for memory. Settings writes receive 503 while maintenance is active; browser shows a clear busy state. Pausing external polling is allowed while usage reception/UI remain available.

```cpp
esp_ota_handle_t handle{};
const esp_partition_t* target = esp_ota_get_next_update_partition(nullptr);
// After policy/identity checks and exclusive admission:
ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ota_begin(target, expectedBytes, &handle));
// For each received chunk: check timeout/remaining length, esp_ota_write.
// On any receive/write/identity failure: esp_ota_abort(handle), no boot selection.
// Only complete validated transfer: esp_ota_end(handle), then esp_ota_set_boot_partition(target).
```

Implement actual error checks/cleanup rather than copying the illustrative macro as unchecked success. UI calls stay in main.

- [ ] **Step 4: Implement effective partition and boot changes.** Use the exact global geometry, require `app_update`, enable `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` in actual generated config (defaults alone do not override an existing sdkconfig). Set a human-readable `PROJECT_VER` of `2.0.0`; status reports current `esp_app_desc` version and available slot capacity. `ota_confirm_boot()` accepts `ESP_OTA_IMG_PENDING_VERIFY` only after successful runtime/display/input/UI/AP/portal/network-task initialization and at least one UI service iteration. Failure/crash/reset before confirmation must leave rollback available. Do not wait for internet, a configured endpoint, or station association. Maintain NVS schema unchanged.

```csv
nvs, data, nvs, 0x9000, 0x16000,
phy_init, data, phy, 0x1f000, 0x1000,
otadata, data, ota, 0x20000, 0x2000,
ota_0, app, ota_0, 0x30000, 0x1e0000,
ota_1, app, ota_1, 0x210000, 0x1e0000,
```

- [ ] **Step 5: Add browser update workflow.** Keep saved settings semantics (blank secrets preserve, explicit clearing only). Add a separate firmware form with version/capacity, `.bin` selection, size check, native progress, upload/verification/restart/reconnect text. Use XHR binary upload with required header and existing same-origin credentials; status fetch stays authenticated. Do not auto-submit upload when a file is selected. Do not label 100% network upload as installation complete before server validation; reconnect polling must be bounded and recover into a clear retry state. Use a black coherent portal style without changing field semantics.

```js
const req = new XMLHttpRequest();
req.open('POST', '/api/firmware');
req.setRequestHeader('Content-Type', 'application/octet-stream');
req.setRequestHeader('X-DeskDuo-Update', '1');
req.upload.onprogress = e => { if (e.lengthComputable) progress.value = 100 * e.loaded / e.total; };
req.send(selectedFile);
```

- [ ] **Step 6: Implement verified packaging and segmented migration.** Parse the built partition table and flasher args, validate exact retained NVS/PHY and both OTA bounds. Include cleared/initialized OTA data as a package segment. Produce application-only `desk-duo-app.bin` for Wi-Fi upload and full initial-install package explicitly labeled as erasing settings. Produce migration metadata/segments for bootloader, partition table, OTA data, and initial app; verify erased padding and every segment hash/byte range. Never migrate a configured device by writing the merged file at 0x0.

`scripts/migrate_ota.py` defaults to printing a dry-run plan. Require `--port` and `--flash` to touch a device. Before writes, read a fresh 4 MiB backup into a private timestamped directory, retain hashes and enough recovery metadata, verify physical chip/flash and package metadata. Invoke esptool on individual validated segments only; ensure none of their sector erase ranges intersects NVS/PHY. Do not print passwords/tokens or backup contents. Document recovery and the new serial application offset. Packaging itself never accesses a serial device.

- [ ] **Step 7: Run focused GREEN, existing regressions, target build, and packaging checks.** Commands use `scripts/pio.py run -d firmware -e cyd`, `scripts/test_firmware.py`, Python unittest, bundled Node tests, actual-LVGL native preview, and `scripts/package_firmware.py`. Report actual app size/slot headroom/effective rollback config. Preview UI should remain unchanged from Task 1 except any documented integration state.

- [ ] **Step 8: Commit and report software verification separately from hardware.** No connected port has been detected yet. Controller owns device migration and live OTA testing after source review. Hardware-only verification: settings preserved after segmented USB migration; successful authenticated Wi-Fi upload/reconnect/hook; failed/truncated upload keeps current boot; controlled failing trial image rolls back with USB recovery available. Never call hardware rollback verified from compilation or mocks.

## Controller Integration and Completion

- [ ] Run task-scoped spec/quality reviews after each task; resolve important findings through implementer and scoped re-review.
- [ ] Run final whole-change review on Astra high and additional independent Luna max review; resolve actionable issues and verify final code.
- [ ] Inspect native renders as evidence of the actual UI, not just the concept image. Show final screenshot(s).
- [ ] If board is connected, fresh backup, migrate validated individual segments, compare private NVS config without outputting secrets, then test one Wi-Fi update and usage hook. Run controlled rollback check only with recovery prepared; record actual result.
- [ ] If hardware is unavailable, deliver reviewed build/update files and exact migration command, with hardware checks explicitly pending. Do not claim firmware is installed.

No calendar implementation and no deployment of Pi/weather/map services in this plan.
