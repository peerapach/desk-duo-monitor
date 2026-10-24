# Google Sheets stock endpoint

`Code.gs` reads a stock table from Google Sheets and returns JSON for the ESP32. The device does not read the Sheet directly. Prices must be entered manually or supplied by another source. This integration does not fetch quotes automatically or establish that prices are current market prices.

## Sheet format

The first row must contain headers, including `symbol` and `price`. Header matching is case-insensitive and trims leading/trailing whitespace. Unknown columns are ignored.

| Header | Required | Meaning |
| --- | --- | --- |
| `symbol` | Yes | Stock symbol, such as `PTT` |
| `price` | Yes | Numeric price; negative values are rejected |
| `label` | No | Display name; defaults to `symbol` when empty |
| `changePercent` | No | Percentage change; `1.56` means `+1.56%` |
| `currency` | No | Currency; defaults to `THB` when empty |
| `quotedAt` | No | Source quote time, supplied as a Sheet Date or ISO datetime with a timezone |

The following table contains simulated data, not current market prices:

| symbol | price | changePercent | quotedAt | label | currency |
| --- | ---: | ---: | --- | --- | --- |
| PTT | 32.5 | 1.56 | 2026-10-04T02:55:00Z | PTT | THB |
| AOT | 60 |  |  | Airports of Thailand | THB |

Formulas are supported when `getValues()` returns a JavaScript number. Numeric strings must use an unambiguous decimal/exponent format, such as `32.5` or `3.25e1`. Empty cells do not become zero, and reported zero values are preserved. The script checks headers and the first 200 data rows, returning at most five nonempty symbols in row order.

Missing `symbol`/`price` headers, duplicate recognized headers, or invalid stock/numeric/date values produce an `invalid_sheet` JSON error instead of fabricated prices.

## Install Apps Script

1. Create a private Google Sheet. Do not make the source Sheet publicly readable.
2. Open **Extensions → Apps Script**. Replace `Code.gs` with `integrations/google-sheets/Code.gs` from this repository and save.
3. Open **Project Settings → Script Properties** and add:

   | Property | Value |
   | --- | --- |
   | `SPREADSHEET_ID` | The part of the Sheet URL between `/d/` and `/edit` |
   | `SHEET_NAME` | The exact tab name shown in the Sheet |
   | `ACCESS_TOKEN` | A random token generated locally |

   Generate a token locally with `openssl rand -hex 32` and enter it directly in Script Properties. Do not send real tokens in chat or put them in shared documentation.
4. Choose **Deploy → New deployment → Web app**. Set **Execute as** to the script owner's account and select **Who has access** according to your account or Workspace policy. For access without a Google login, the deployment must allow unauthenticated web-app requests, which is available only if Workspace policy permits it. This does not make the source Sheet public.
5. Deploy and keep the web-app URL ending in `/exec` for device configuration. Do not expose the token in logs or screenshots.

## Call the endpoint

Send a GET request to the deployed URL with an `accessToken` query parameter:

```text
https://script.google.com/macros/s/DEPLOYMENT_ID/exec?accessToken=YOUR_TOKEN
```

Enter the real token only on your device or in your own test tools. URL-encode special characters. The endpoint checks the token before calling `SpreadsheetApp` and accepts the token only through the Apps Script event's query parameters.

See [`examples/stocks-response.json`](../../examples/stocks-response.json) for a response. `generatedAt` is the response creation time. Optional `quotedAt` is the source quote time; do not invent it when unknown. Empty price and `changePercent` fields are omitted.

Apps Script `ContentService` cannot set the HTTP response status, so errors may arrive with HTTP 200. Callers must inspect the JSON and `error.code`. The only error codes are `auth_required`, `configuration_error`, `invalid_sheet`, and `source_unavailable`. Exception messages, tokens, and private IDs are not returned. The ESP32 client must follow Google's ContentService redirects.

## Token rotation and price sources

To rotate a token, generate a new value locally, update `ACCESS_TOKEN` in Script Properties, and update the ESP32 to match. The previous token then stops working. Avoid serial logging of endpoint URLs because the query contains the token.

This script only reads and validates the Sheet. It does not automatically fetch or populate prices through `GOOGLEFINANCE` or external services. Choose a suitable source for your symbols and verify its `quotedAt` timestamp yourself.
