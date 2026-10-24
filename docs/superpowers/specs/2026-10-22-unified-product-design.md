# Desk Duo unified product design

Approved: user requested both ESP32 and Web Admin mockups, selected the uploaded monochrome white scalloped Codex cloud with black `>_`, retained the cute orange Claude, and explicitly said “OK ตามนี้เยี่ยมมาก ทำ firmware ได้เลย”. Approval covers implementing the pictured product, building firmware; no board deployment is claimed.

## Visual contract

- Desk Duo branding, black background, ivory typography, quiet mint selection; orange Claude and negative stock moves. No infinity/ribbon or invented Codex robot.
- ESP32 portrait 240×320, fixed four-item navigation Duo / Weather / Stocks / Setup with consistent 60×44 touch regions, content above y256 and visible IP footer. TH/EN across device and portal. All label/icon areas must activate their parent target.
- Duo: cute rounded orange block Claude asset and exact user Codex white cloud logo; distinct provider colors, two usage windows; saved Codex weekly-only hides absent 5h without inventing a zero.
- Weather: substantial small raster weather illustration, daytime sun, night moon, cloud and rain. Clear temperature/location/condition/humidity, next three forecast entries with times and source timestamp. Missing data stays unknown; stale source must be visible. No false “fresh” from a newly received old observation.
- Stocks: retain approved direct TH/US tickers, compact sparklines, Today/5 days, quote/stale behavior. Match navigation and typography.
- Setup: brightness slider uses configured base, saved two-period day/night schedule, language controls and network state; keep local credential reveal capability available without conflating with connection status.

## Web Admin contract

- Six navigable sections: Display, AI usage, Weather, Stocks, Wi-Fi, Firmware. Desktop sidebar; mobile section selector. No external fonts/scripts/CDN dependencies.
- Show actual saved configuration distinctly from editable drafts; do not fake measurements or success. Changing section/language preserves drafts. Save and discard are explicit, status errors stay visible.
- Display: 5–100% base slider, auto toggle, configurable day/night start times; day full base, night half with 5% minimum; language TH/EN.
- AI usage: provider identities, Codex Both/Weekly selector, usage hook endpoint and configured-token state without revealing secrets.
- Weather: friendly Open-Meteo location name + latitude/longitude controls produce a matching endpoint; preserve arbitrary endpoint/headers/mappings in Advanced. Infer coordinates only from a recognized endpoint; never overwrite custom settings on load. Distinguish observation time from receipt age. A field named location is not proof of the endpoint location.
- Stocks: retain Yahoo provider, five optional market/ticker/show rows, range, refresh minimum 300s, advanced legacy JSON. No Google Sheets requirement.
- Wi-Fi: saved SSID, optional password replacement, empty preserves secret, explicit clearing retains its existing meaning; saving only affected settings must not overwrite other drafts.
- Firmware: actual current version, .bin chooser, actual selected size, review action before upload, real progress/errors/restart states. Preserve OTA validation/auth/origin/concurrency limits.

## Data and compatibility

Add optional `Weather.isDay`, `hourlyWeatherCodes[4]`, `hourlyIsDay[4]` mapped from Open-Meteo. Missing fields remain optional and legacy stored mappings remain valid. Retain bounded four-sample storage, select first observations at/after source updatedAt when known rather than midnight samples. Unknown day/night never silently becomes a moon. Source timestamps stay normalized UTC and are rendered using source offset. Existing source, usage, stock, brightness and OTA contracts remain intact.

## Acceptance scenarios

U1 Given either language, when each nav target is tapped at icon/text/edge, then it opens the intended page without footer overlap.
U2 Given dual or weekly-only Codex data, when configured mode changes, then only appropriate windows display and usage hook still updates values.
W1 Given day, night, rain or missing weather, then corresponding artwork/unknown state displays with source observation time, and stale observation is not labeled current.
W2 Given a long hourly response starting before observation, then at most four nonpast aligned entries are retained; optional code/day arrays stay aligned.
P1 Given saved settings, when section/language changes after edits, then drafts survive; successful scoped save updates readback, failure preserves saved values and reports error.
P2 Given recognized Open-Meteo URL, when coordinates are edited and saved, then URL uses those coordinates; custom endpoints are preserved until an explicit provider change.
P3 Given 320px mobile viewport, then no horizontal overflow and navigation/inputs/buttons remain reachable with 44px targets.
R1 Given existing sources/secrets/calibration/OTA state, then legacy regression suites remain passing; package fits target OTA partition and version advances to 2.2.0.

## Evidence and limits

Image mockups contain sample data; firmware must use real runtime values. Physical touch, live weather accuracy against the user's exact location, live Yahoo service availability and OTA installation require hardware/network verification; local checks must not be described as those checks.
