# Direct stocks and small charts implementation plan

> **For agentic workers:** Use subagent-driven-development for scoped implementation, review and final verification.

**Goal:** Deliver offline-built Desk Duo firmware with Yahoo stock charts and stock watchlist settings.
**Architecture:** Extend the existing stock source config compatibly, parse bounded
Yahoo chart JSON using the portable core, aggregate per-symbol fetch results on
the existing network task, and render a separate LVGL stock view. Admin continues
using authenticated sparse configuration patches.
**Tech Stack:** ESP-IDF 5.5.3, C++17/cJSON, LVGL 9.2.2, embedded HTML/JS, native tests.
**Spec:** docs/superpowers/specs/2026-10-22-stock-sparklines-design.md

## Global constraints

- Board offline: no device calls or OTA; preserve existing settings compatibility.
- Shell starts rtk; no system temp files; scratch under repository .worktrees.
- Reuse clean feature checkout codex/desk-duo; do not touch unrelated graft/.
- No new services, API keys, new dependencies or paid data provider enrollment.
- Preserve authenticated route, NVS commit checks, verified TLS, usage and weather.

## Planning Discovery

READY — LOCAL IMPACT with external-provider limitation.
- core.hpp has SourceConfig shared by weather/stocks and fixed five Stock items;
  core.cpp parseSourcePatch/configJson/sameSource and runtime.hpp
  completeSerialization must all include new stock-only fields.
- network.cpp fetch already bounds 16KiB body / 32KiB wire per request and 10s
  deadline, follows bounded redirects, strips headers across origins. Polling
  uses model generation; use this exact transport for direct Yahoo requests.
- runtime_patch persists once before model.applyConfig; config generation cancels
  stale requests. UI period change must acquire Maintenance::Settings as portal
  does; no NVS writes for drawing or normal ticks.
- ui.cpp content y24..255, IP y259, bottom nav276..319. weather_view.cpp provides
  separate view + LVGL draw-line precedent. Use four compact stock rows and local
  pagination rather than retain the current three-row mascot layout.
- assets.cpp admin preserves blank secret editors and sparse source patch state;
  add stocks controls without regenerating forms on locale changes.
- Yahoo public chart probe returned429; parser/poll tests use explicit fixtures,
  and release notes disclose that live success and board behavior are unverified.

## Settled interfaces

Enums: StockProvider {Json,Yahoo}, StockChartRange {Day,FiveDays}.
StockSelection {std::string symbol; bool enabled=true;}.
SourceConfig adds provider (Json), watchlist array5, watchCount (0), chartRange
(Day). JSON stock keys: provider=json|yahoo, watchlist=[{symbol,enabled}],
chartRange=1d|5d. Weather rejects these keys and nondefault stock fields.
Canonical tickers uppercase ASCII letters/digits/dot/hyphen, start letter/digit,
max20; .BK suffix for Thai; no duplicate symbols. Yahoo requires refresh>=300
seconds and >=1 enabled stock before polling (empty/all-disabled is valid off).
Stock adds history array32 of {int64_t at; optional<double> close}, historyCount,
chartRange and stale bool; Stocks adds partialFailure bool.
Snapshot adds stockChartRange and stocksConfigured for the view.
parseYahooChart(json,symbol,range) -> ParseResult<Stock>, implemented in core.cpp
to reuse strict bounded JSON helpers. stockChartUrl(symbol,range) -> string in
new stock_source.cpp: fixed query1.finance.yahoo.com/v8/finance/chart/<symbol>,
1d/5m or 5d/30m, includePrePost=false.
pollYahooStocks(source, previous, nowEpoch, fetcher) -> ParseResult<Stocks>,
new stock_source.{hpp,cpp}, portable injectable fetcher accepts SourceConfig and
returns optional<string>. Generated Yahoo request has no legacy custom headers.

### Task 1: Config, strict Yahoo parser and polling contract (S1–S3)

Files: core.{hpp,cpp}, runtime.hpp, stock_source.{hpp,cpp},
tests/stock_source_test.cpp, tests/firmware_core_test.cpp,
scripts/test_firmware.py, firmware/main/CMakeLists.txt.
- [x] Add meaningful failing config/parser/poller/generation tests; observe RED.
- [x] Implement settled interfaces, serialization completeness and generation
      invalidation. Retain strict JSON validation and cap32 sampled points.
- [x] Parse Yahoo meta.symbol/currency/regularMarketPrice/regularMarketTime,
      chartPreviousClose (day), timestamp and indicators.quote[0].close arrays;
      no null-to-zero conversion. Day reference must be finite positive; missing
      reference gives unknown change. FiveDays uses first valid close.
- [x] Preserve gaps during sampling; flat lines render validly. Reject identity
      mismatch, bad times/nonfinite/negative prices; reject malformed/error JSON.
- [x] Aggregation copies matching prior stock on failure, marks stale; no fresh
      success on all-failure. All disabled yields empty valid configured state.
- [x] Run portable regression tests and independent scoped task review.

### Task 2: Transport integration (S3)

Files: network.cpp plus test seam as needed.
- [x] Test disabled/Yahoo/JSON dispatch via portable polling tests from Task1.
- [x] Wire pollYahooStocks into existing network task and runtime_stocks.
      Use existing fetch sequentially, generation checks and300s minimum,
      default User-Agent DeskDuo/1.0. No retry/bypass of429 and no cert weakening.
- [x] Target compile proves SDK binding; review integrated call path with Task1.

### Task 3: LVGL stock view and native evidence (S4)

Files: stock_view.{hpp,cpp}, ui.cpp, tests/native_ui/preview.cpp,
tests/native_ui/CMakeLists.txt, firmware/main/CMakeLists.txt.
- [x] Add native RED tests for localized stock header,4 rows, ranges,5th item,
      real pointer hitboxes, stale/empty and flat/gapped charts.
- [x] Separate stock_view create/update; persistent point storage or draw events;
      don't leave borrowed stack data with LVGL. Remove Stocks mascots.
- [x] Stock header replaces Desk Duo only on stocks page; preserve other pages.
      Today/5D effective touch targets44px; period callback writes sparse patch
      with MaintenanceLease Settings, feedback on failure, triggers refetch.
- [x] Render Thai/English native PPMs and inspect; preserve all existing tests.
- [x] Independent scoped spec/quality review.

### Task 4: Web stock settings (S5)

Files: assets.cpp, tests/portal.test.cjs, tests/portal_dom.cjs.
- [x] Write RED tests for loaded stock fields + sparse provider/list/range save,
      invalid tickers/duplicates/limit, enabled toggles, dirty locale preservation.
- [x] Add organized Yahoo stock section with max5 editable market/ticker/enabled
      rows and saved readback; keep generic JSON option for legacy config.
      Remove Google Sheets wording. No broad unrelated admin redesign this task.
- [x] Locale updates labels without clearing inputs; no secret roundtrip.
- [x] Run portal regression and offline browser checks, inspect narrow screenshots.
- [x] Independent scoped review.

### Task 5: Release verification (S6)

Files: README.md, firmware/README.md, docs/protocol.md,
examples/device-config.example.json and new stock fixture as relevant.
- [x] Document direct Yahoo setup, unofficial/delayed data,429/stale states,
      graph-period meaning, saved fields and legacy compatibility.
- [x] All portable/portal/Python/native suites, actual ESP32 target build,
      package_firmware.py to repository output and inspect image/manifest hashes.
- [x] Broad independent final review and additional Luna max checkpoint.
- [x] Deliver firmware link and clear offline/live/OTA limitation; do not deploy.
