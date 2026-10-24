# Thai and US stocks on Desk Duo

## Yahoo Finance setup

After installing the firmware, open the web admin and select **Stock provider → Yahoo Finance**. Choose market **TH** and enter a symbol such as `PTT` or `ADVANC`; choose **US** for symbols such as `AAPL` or `MSFT`.

Clear a symbol field to remove an entry. Use **Show** to enable or hide it. Up to five entries can be saved. Select **Today** or **5 days**, choose a refresh interval of at least 300 seconds, and save. Saved values are shown separately from draft fields. Saving stocks preserves Wi-Fi and usage settings.

The ESP32 fetches data directly, without Sheets, a Mac, or a Pi as an intermediary. Yahoo access is unofficial: data may be delayed, response formats may change, and requests may be rate-limited with HTTP 429. Parser and poller tests use fixtures; they do not establish live quote accuracy.

## Charts and percentages

The on-device Today / 5 days control changes the chart range and saves it. Failed reads show stale or unknown data rather than fabricated zero prices or charts.

Today percentage changes use `chartPreviousClose` from the one-day response. The 5 days percentage compares against the first displayed closing-price sample; it is not a dividend-inclusive total return. Charts retain at most 32 points and preserve gaps in missing data.

```json
{"stocks":{"provider":"yahoo","watchlist":[{"symbol":"PTT.BK","enabled":true},{"symbol":"AAPL","enabled":true}],"chartRange":"1d","refreshSeconds":300}}
```

## Custom JSON endpoints

Existing settings without a `provider` continue to use the JSON endpoint. Select **JSON endpoint (legacy)** and expand URL/headers in the web admin. Sources are not migrated automatically. This mode has no chart when the source supplies prices only.

For Wi-Fi firmware updates, select only `artifacts/desk-duo-app.bin` in the Firmware section. The merged `desk-duo-cyd.bin` image is for initial USB installation, not Wi-Fi updates. Verify live quotes, touch interaction, and OTA behavior on the physical device.
