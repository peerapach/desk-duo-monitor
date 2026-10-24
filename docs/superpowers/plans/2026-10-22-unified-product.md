# Desk Duo unified product implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use subagent-driven-development to implement this plan task-by-task. User approved the visual design and explicitly authorized implementation in this session.

**Goal:** Ship one consistent ESP32 and embedded Web Admin interface matching the approved design.
**Architecture:** Keep existing APIs/settings/auth, source polling and LVGL runtime. Add bounded optional weather presentation fields, replace static image assets, and organize the existing portal without replacing its state/save machinery.
**Tech Stack:** ESP-IDF C++17, LVGL9, embedded HTML/CSS/JS, Node, portable C++ tests and native LVGL screenshots.
**Spec:** docs/superpowers/specs/2026-10-22-unified-product-design.md

## Global Constraints

- Use current `codex/desk-duo` workspace containing previous authorized uncommitted stock work. Preserve all baseline work; never inspect/stage unrelated graft/.
- Every shell command starts with rtk. Scratch lives under repo .worktrees; no OS temporary folder.
- No credentials in logs or fixtures. No deployment or remote publication as part of build.
- TH/EN, actual saved readback, optional weather fields, current API and stock/brightness/OTA semantics are mandatory.
- Firmware version 2.2.0; OTA partition capacity 1,966,080 bytes.

## Planning Discovery

READY — LOCAL IMPACT
- `ui.cpp` owns page/nav geometry and setup brightness; `weather_view.cpp` owns weather illustration and source-time presentation; `stock_view.cpp` already supplies tested sparklines. `character_assets.cpp` consumes compiled RGB565 pixels.
- `assets.cpp` owns entire portal HTML and existing scoped dirty/readback save functions; tests/portal_dom.cjs exercises it in a real browser.
- `core.hpp:Weather` is bounded optional data; core.cpp parseWeather resolves default+stored mappings and currently takes first four samples. Weather diagnostics JSON exposes receipt age but needs observation time/day fields.
- Core weather producer is independent of portal and UI once exact names are established: optional<bool> isDay; arrays hourlyWeatherCodes and hourlyIsDay. Runtime copies remain bounded.
- Native UI build links ui/weather/stock/assets; all UI touches must preserve layout/touch/brightness tests. Portable core and portal regression commands already exist.
- Asset prerequisite: root supplies `firmware/assets/claude-approved.png`, `codex-logo.png`, and `weather-atlas.png`; UI task compiles them to flash-resident pixels. Native screenshots and actual ESP32 build prove feasibility and partition fit.

### Task 1: Weather presentation data
Owner: weather-data worker. Files core.hpp/core.cpp/weather_diagnostics.{hpp,cpp}, relevant portable tests, example config/protocol weather docs only.
Consumes source mapping and bounded JSON. Produces optional isDay, hourlyWeatherCodes[4], hourlyIsDay[4], source-time aligned arrays and diagnostics updatedAt/utcOffsetSeconds/isDay.
- [x] Add and run failing portable cases for optional day flags/codes, invalid values, old mappings and past-hour skipping with aligned arrays.
- [x] Add optional mappings `isDay: current.is_day`, `hourlyWeatherCodes: hourly.weather_code`, `hourlyIsDay: hourly.is_day`; validate present values and preserve missing optional semantics.
- [x] Parse bounded chronological forecast slice relative to updatedAt; preserve existing no-time behavior; test bad/missing elements and unknown offsets.
- [x] Extend safe diagnostic JSON with source updatedAt/offset/day. Run `rtk proxy python3 scripts/test_firmware.py`; record report under .worktrees/unified-product/task1-report.md.

### Task 2: ESP32 visual family and image pipeline
Owner: device worker. Files ui.cpp, weather_view.cpp, character_assets.{hpp,cpp}, compile_character_assets.mjs, new weather assets implementation/compiler if needed, native UI CMake/tests. May add target source to firmware/main/CMakeLists.txt if required.
Consumes Task1 Weather exact field names and root-provided assets. Produces actual LVGL UI and native preview images; no backend/config API changes.
- [x] Extend native preview assertions for all nav target zones, both languages, weekly-only and weather day/night/rain/stale views; run relevant failing checks first.
- [x] Compile selected Claude and exact user logo to small RGB565 assets; weather six-tile atlas order clear-day, partial-day, clear-night, partial-night, cloudy, rain, render at suitable 80–96px and tiny forecast size. Flash residency, no full-frame runtime PNG decoder.
- [x] Match approved header/nav/color/usage layout, keep y256 IP footer and y276 60×44 navigation, preserve brightness drag/persist semantics; add Setup language control and readable schedule/network without removing credential access.
- [x] Rebuild native preview and inspect actual snapshots for Thai clipping, footer overlaps and touch regression. Record report and paths under .worktrees/unified-product/task2-report.md.

### Task 3: Embedded responsive Web Admin
Owner: admin worker. Files assets.cpp and portal tests only (coordinate new asset includes with root/device worker). Preserve APIs, globals and old test hooks when feasible.
Consumes real config/readback and Task1 new safe diagnostic fields. Produces six sections, accessible navigation/controls, scoped saves, preserved drafts, actual states and offline assets.
- [x] Add failing DOM checks for six-section navigation, mobile overflow, language+section draft survival, scoped submit and readback/failure.
- [x] Reorganize existing elements into section containers using semantic HTML and existing form ownership, no per-section cloning of editors. Add desktop/sidebar and mobile selector, base/mint CSS and real saved strip.
- [x] Keep display and settings API ownership correct when moving usage mode. Add explicit weather mode/coordinates editor that derives recognized Open-Meteo URLs, includes new optional forecast fields, keeps custom endpoint in Advanced and does not mutate on load.
- [x] Add discard action restoring persisted snapshot only for current section. Add OTA review step showing actual filename/size before existing validation+upload. No fake live snapshots or hardcoded versions.
- [x] Run Node unit+browser DOM regressions, capture desktop/mobile screenshots, report .worktrees/unified-product/task3-report.md.

### Task 4: Integration, review and deliver firmware
Owner: root integration worker after all joins ready.
- [x] Independently review Tasks1–3 against spec and quality; return material findings to owners and rerun affected tests.
- [x] Advance PROJECT_VER to2.2.0, update relevant docs and image attribution/source provenance; build target using scripts/pio.py run -d firmware.
- [x] Run portable firmware, portal Node+DOM, Python tools and native LVGL checks on integrated working tree.
- [x] Final whole-change review and independent Luna max checkpoint. Verify correction evidence before packaging.
- [x] Package using scripts/package_firmware.py, verify manifest/app identity/hash/partition and .bin equality; deliver .bin and real UI previews, state hardware/live/OTA limits clearly.
