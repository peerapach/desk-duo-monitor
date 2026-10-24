# Desk Duo direct stocks and small charts

Approved: user selected the portrait sparkline mockup, asked for top title หุ้น
instead of Desk Duo and removal of the TH / US heading, then authorized firmware
implementation on 2026-10-22. Board is offline: local delivery only, no OTA.

## Outcome

- Stocks page has a localized หุ้น / Stocks title, clock, Today / 5 days controls,
  four compact rows (paginate a fifth), ticker, small line chart, price, currency,
  and signed percentage change. Keep existing 44px bottom navigation and LAN IP.
- Web settings manage up to five Thai/US stocks, including enabled state, saved
  readback, Yahoo provider, chart period and refresh. Thai symbols use .BK.
  The existing generic JSON source remains as a compatibility option; no Google
  Sheet is required or advertised as the stock setup path.
- ESP32 fetches Yahoo chart JSON directly, with one selected-period request per
  enabled ticker. Keep verified HTTPS, existing source byte/deadline/redirect
  limits, normal usage hooks, weather, brightness, language and OTA behavior.
- No fake charts or zero prices for missing data. Retain last known values with
  stale indication on failure, including per-symbol partial failures. No busy
  retry on HTTP 429. Provider requests may fail; Yahoo is an unofficial
  integration, not guaranteed real-time. Probe from this Mac returned HTTP 429.

## Acceptance scenarios

S1: Existing stored config lacking stock extensions restores unchanged as JSON.
    New stock fields round-trip; invalid/duplicate/overlong or >5 tickers are
    rejected atomically. Sparse unrelated saves preserve them and all secrets.
S2: A selected Yahoo ticker produces only a fixed HTTPS Yahoo URL; JSON with
    valid metadata and aligned timestamp/close samples yields <=32 chart points.
    Missing/null samples remain gaps. Invalid timestamps, wrong symbol/currency,
    malformed shape/errors and oversized data are rejected. Day percentage uses
    the one-day chart reference; 5-day percentage uses first valid plotted close.
S3: Poll only enabled symbols in order. All-failure preserves the previous
    snapshot and marks stale; partial failure retains matching old values marked
    stale while accepting healthy symbols. Generation mismatch cannot accept old
    provider/watchlist/range results. Weather and usage keep existing ownership.
S4: Native LVGL checks demonstrate four rows, actual line draw, gap/flat/empty
    states, fifth-item pagination, localized title, period clicks and unchanged
    bottom nav/IP geometry. Switching period persists only chartRange and triggers
    a request through the existing config generation path.
S5: Admin reads current settings, edits/saves provider, ticker/enabled list,
    range/refresh, validates duplicates/limit, preserves dirty inputs on language
    changes, and does not send unrelated secrets. Saved/unsaved state is explicit.
S6: Portable, portal, native UI, target build and release packaging checks pass.
    Release app is suitable for existing Wi-Fi OTA partition and has a manifest
    with hashes. Board connectivity, real touch, live quotes and OTA are not run.
