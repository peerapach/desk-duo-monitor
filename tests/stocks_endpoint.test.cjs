'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const test = require('node:test');
const vm = require('node:vm');

const sourcePath = path.join(__dirname, '..', 'integrations', 'google-sheets', 'Code.gs');
const source = fs.existsSync(sourcePath) ? fs.readFileSync(sourcePath, 'utf8') : '';
const now = new Date('2026-10-04T05:06:07.000Z');
const token = 'local-test-token';
const header = ['symbol', 'price', 'changePercent', 'quotedAt', 'label', 'currency'];

function makeServices({
  rows = [header],
  properties = {},
  sheetExists = true,
  openError,
  rangeError,
  lastRow = rows.length,
  lastColumn = rows.reduce((max, row) => Math.max(max, row.length), 0),
} = {}) {
  const configured = {
    SPREADSHEET_ID: 'private-sheet-id',
    SHEET_NAME: 'Stocks',
    ACCESS_TOKEN: token,
    ...properties,
  };
  const metrics = { openCount: 0, openIds: [], tabNames: [], ranges: [] };
  const sheet = {
    getLastRow() { return lastRow; },
    getLastColumn() { return lastColumn; },
    getRange(row, column, numRows, numColumns) {
      metrics.ranges.push([row, column, numRows, numColumns]);
      if (rangeError) throw rangeError;
      return {
        getValues() {
          return Array.from({ length: numRows }, (_, rowOffset) =>
            Array.from({ length: numColumns }, (_, columnOffset) => {
              const value = rows[rowOffset]?.[columnOffset];
              return value === undefined ? '' : value;
            }));
        },
      };
    },
  };
  const context = {
    PropertiesService: {
      getScriptProperties() {
        return { getProperty(name) { return Object.hasOwn(configured, name) ? configured[name] : null; } };
      },
    },
    SpreadsheetApp: {
      openById(id) {
        metrics.openCount += 1;
        metrics.openIds.push(id);
        if (openError) throw openError;
        return {
          getSheetByName(name) {
            metrics.tabNames.push(name);
            return sheetExists ? sheet : null;
          },
        };
      },
    },
    ContentService: {
      MimeType: { JSON: 'application/json' },
      createTextOutput(content) {
        return {
          content,
          mimeType: undefined,
          setMimeType(mimeType) { this.mimeType = mimeType; return this; },
        };
      },
    },
  };
  vm.runInNewContext(source, context, { filename: sourcePath });
  return { context, metrics };
}

function build(rows, buildTime = now) {
  const { context } = makeServices();
  return JSON.parse(JSON.stringify(context.buildStocksPayload(rows, buildTime)));
}

function callHandler(event, services = makeServices()) {
  const output = services.context.doGet(event);
  return { payload: JSON.parse(output.content), mimeType: output.mimeType, output };
}

function authorizedEvent() {
  return { parameter: { accessToken: token } };
}

test('buildStocksPayload preserves a zero price and formats generatedAt and Date quotes as UTC ISO', () => {
  const result = build([
    ['symbol', 'price', 'changePercent', 'quotedAt'],
    ['PTT', 0, '', new Date('2026-10-04T03:00:00Z')],
  ]);

  assert.equal(result.generatedAt, '2026-10-04T05:06:07.000Z');
  assert.deepEqual(result.items, [{
    symbol: 'PTT', label: 'PTT', price: 0, currency: 'THB', quotedAt: '2026-10-04T03:00:00.000Z',
  }]);
});

test('buildStocksPayload matches trimmed case-insensitive reordered headers and ignores unknown columns', () => {
  const result = build([
    [' Extra ', ' CURRENCY ', ' quotedAt ', 'CHANGEpercent', ' label ', ' PRICE ', ' SYMBOL '],
    ['ignored', 'USD', '2026-10-04T10:00:00+07:00', '-1.25', ' PTT Public ', '32.5', ' PTT '],
  ]);

  assert.deepEqual(result.items, [{
    symbol: 'PTT', label: 'PTT Public', price: 32.5, changePercent: -1.25,
    currency: 'USD', quotedAt: '2026-10-04T03:00:00.000Z',
  }]);
});

test('buildStocksPayload ignores duplicate unknown headers even when their names match object properties', () => {
  const result = build([
    ['symbol', 'price', 'constructor', 'constructor', 'toString'],
    ['PTT', 32.5, 'ignored', 'ignored', 'ignored'],
  ]);

  assert.deepEqual(result.items, [{ symbol: 'PTT', label: 'PTT', price: 32.5, currency: 'THB' }]);
});

test('buildStocksPayload skips trailing blank rows and accepts a header-only table', () => {
  const result = build([
    ['symbol', 'price'],
    ['PTT', 32.5],
    ['', ''],
    ['   ', ''],
  ]);

  assert.deepEqual(result.items, [{ symbol: 'PTT', label: 'PTT', price: 32.5, currency: 'THB' }]);
  assert.deepEqual(build([['symbol', 'price']]).items, []);
});

test('buildStocksPayload rejects an empty sheet because required headers are absent', () => {
  assert.deepEqual(build([]), { schemaVersion: 1, error: { code: 'invalid_sheet' } });
});

test('buildStocksPayload rejects a missing symbol header', () => {
  assert.equal(build([['price'], [32.5]]).error.code, 'invalid_sheet');
});

test('buildStocksPayload rejects a missing price header', () => {
  assert.equal(build([['symbol'], ['PTT']]).error.code, 'invalid_sheet');
});

test('buildStocksPayload rejects duplicated recognized headers after trimming and case folding', () => {
  assert.equal(build([['symbol', ' PRICE ', 'price'], ['PTT', 32, 33]]).error.code, 'invalid_sheet');
});

test('buildStocksPayload accepts formula results returned as numeric cell values', () => {
  const result = build([['symbol', 'price'], ['PTT', 32.5]]);

  assert.deepEqual(result.items, [{ symbol: 'PTT', label: 'PTT', price: 32.5, currency: 'THB' }]);
});

test('buildStocksPayload rejects nonfinite, boolean, locale-ambiguous, nonnumeric, and negative prices', () => {
  for (const invalidPrice of [Infinity, NaN, true, '1,234.50', 'not a price', -0.01]) {
    const result = build([['symbol', 'price'], ['PTT', invalidPrice]]);
    assert.equal(result.error.code, 'invalid_sheet', `expected ${String(invalidPrice)} to be rejected`);
  }
});

test('buildStocksPayload accepts trimmed decimal and exponent numeric strings', () => {
  const result = build([
    ['symbol', 'price', 'changePercent'],
    ['PTT', ' 3.25e1 ', '-1.2E-1'],
  ]);

  assert.deepEqual(result.items, [{
    symbol: 'PTT', label: 'PTT', price: 32.5, changePercent: -0.12, currency: 'THB',
  }]);
});

test('buildStocksPayload omits blank price and changePercent rather than turning them into zero', () => {
  const result = build([
    ['symbol', 'price', 'changePercent'],
    ['PTT', '', '   '],
  ]);

  assert.deepEqual(result.items, [{ symbol: 'PTT', label: 'PTT', currency: 'THB' }]);
});

test('buildStocksPayload rejects invalid nonblank changePercent values', () => {
  for (const changePercent of [true, Infinity, '1,5', 'gain']) {
    assert.equal(
      build([['symbol', 'price', 'changePercent'], ['PTT', 10, changePercent]]).error.code,
      'invalid_sheet',
    );
  }
});

test('buildStocksPayload defaults blank label and currency to symbol and THB', () => {
  const result = build([
    ['symbol', 'price', 'label', 'currency'],
    [' PTT ', 32.5, '  ', ''],
  ]);

  assert.deepEqual(result.items, [{ symbol: 'PTT', label: 'PTT', price: 32.5, currency: 'THB' }]);
});

test('buildStocksPayload omits optional fields when their headers are absent', () => {
  const result = build([['symbol', 'price'], ['PTT', 32.5]]);

  assert.deepEqual(result.items, [{ symbol: 'PTT', label: 'PTT', price: 32.5, currency: 'THB' }]);
});

test('buildStocksPayload rejects quotedAt strings without a timezone or with an invalid date', () => {
  for (const quotedAt of [
    '2026-10-04',
    '2026-10-04T03:00:00',
    '2026-02-30T03:00:00Z',
    'not-a-date',
  ]) {
    assert.equal(
      build([['symbol', 'price', 'quotedAt'], ['PTT', 10, quotedAt]]).error.code,
      'invalid_sheet',
      `expected ${quotedAt} to be rejected`,
    );
  }
});

test('buildStocksPayload rejects an invalid Date quote', () => {
  assert.equal(build([['symbol', 'price', 'quotedAt'], ['PTT', 10, new Date('invalid')]]).error.code, 'invalid_sheet');
});

test('buildStocksPayload omits a blank quotedAt', () => {
  const result = build([['symbol', 'price', 'quotedAt'], ['PTT', 10, '  ']]);

  assert.deepEqual(result.items, [{ symbol: 'PTT', label: 'PTT', price: 10, currency: 'THB' }]);
});

test('buildStocksPayload skips blank symbols and preserves source order for the first five items', () => {
  const rows = [['symbol', 'price']];
  for (let index = 1; index <= 7; index += 1) rows.push([`S${index}`, index]);
  rows.splice(2, 0, ['', 99]);

  assert.deepEqual(build(rows).items.map((item) => item.symbol), ['S1', 'S2', 'S3', 'S4', 'S5']);
});

test('buildStocksPayload stops after five included rows without validating later rows', () => {
  const result = build([
    ['symbol', 'price', 'quotedAt'],
    ['S1', 1, ''], ['S2', 2, ''], ['S3', 3, ''], ['S4', 4, ''], ['S5', 5, ''],
    ['S6', true, 'not a date'],
  ]);

  assert.deepEqual(result.items.map((item) => item.symbol), ['S1', 'S2', 'S3', 'S4', 'S5']);
});

test('buildStocksPayload limits its pure input to 200 data rows', () => {
  const rows = [['symbol', 'price']];
  for (let index = 1; index <= 200; index += 1) rows.push(['', '']);
  rows.push(['TOO_LATE', true]);

  const result = build(rows);
  assert.deepEqual(result.items, []);
});

test('buildStocksPayload rejects symbols longer than 32 characters or containing control characters', () => {
  for (const symbol of ['S'.repeat(33), 'PTT\nALT', 'PTT\u0085ALT']) {
    assert.equal(build([['symbol', 'price'], [symbol, 10]]).error.code, 'invalid_sheet');
  }
});

test('buildStocksPayload rejects labels longer than 64 characters or containing control characters', () => {
  for (const label of ['L'.repeat(65), 'PTT\u0000ALT']) {
    assert.equal(build([['symbol', 'price', 'label'], ['PTT', 10, label]]).error.code, 'invalid_sheet');
  }
});

test('buildStocksPayload rejects currencies longer than 8 characters or containing control characters', () => {
  for (const currency of ['C'.repeat(9), 'THB\tUSD']) {
    assert.equal(build([['symbol', 'price', 'currency'], ['PTT', 10, currency]]).error.code, 'invalid_sheet');
  }
});

test('doGet returns auth_required without opening the spreadsheet when accessToken is absent', () => {
  const services = makeServices();
  const { payload } = callHandler({ parameter: {} }, services);

  assert.equal(payload.error.code, 'auth_required');
  assert.equal(services.metrics.openCount, 0);
});

test('doGet returns auth_required without opening the spreadsheet for a wrong token', () => {
  const services = makeServices();
  const { payload } = callHandler({ parameter: { accessToken: 'wrong-token' } }, services);

  assert.equal(payload.error.code, 'auth_required');
  assert.equal(services.metrics.openCount, 0);
});

test('doGet does not accept a token supplied outside the Apps Script query parameter', () => {
  const services = makeServices();
  const { payload } = callHandler({ parameter: {}, headers: { Authorization: `Bearer ${token}` } }, services);

  assert.equal(payload.error.code, 'auth_required');
  assert.equal(services.metrics.openCount, 0);
});

test('doGet reports missing Script Properties as configuration_error', () => {
  for (const property of ['ACCESS_TOKEN', 'SPREADSHEET_ID', 'SHEET_NAME']) {
    const services = makeServices({ properties: { [property]: '' } });
    const event = { parameter: { accessToken: token } };

    assert.equal(callHandler(event, services).payload.error.code, 'configuration_error', property);
    assert.equal(services.metrics.openCount, 0);
  }
});

test('doGet returns invalid_sheet when the configured tab does not exist', () => {
  const services = makeServices({ sheetExists: false });
  const { payload } = callHandler(authorizedEvent(), services);

  assert.equal(payload.error.code, 'invalid_sheet');
  assert.deepEqual(services.metrics.openIds, ['private-sheet-id']);
  assert.deepEqual(services.metrics.tabNames, ['Stocks']);
});

test('doGet reads only the header and first 200 data rows with getRange', () => {
  const rows = [['symbol', 'price']];
  for (let index = 1; index <= 450; index += 1) rows.push([`S${index}`, index]);
  const services = makeServices({ rows, lastRow: 451, lastColumn: 2 });
  const { payload } = callHandler(authorizedEvent(), services);

  assert.equal(payload.items.length, 5);
  assert.deepEqual(services.metrics.ranges, [[1, 1, 201, 2]]);
});

test('doGet returns invalid_sheet for required headers missing from the selected table', () => {
  const services = makeServices({ rows: [['label', 'price'], ['PTT', 32.5]] });

  assert.equal(callHandler(authorizedEvent(), services).payload.error.code, 'invalid_sheet');
});

test('doGet redacts upstream exceptions and returns only source_unavailable', () => {
  const services = makeServices({ openError: new Error(`upstream failed for private-sheet-id and ${token}`) });
  const { payload, output } = callHandler(authorizedEvent(), services);

  assert.deepEqual(payload, { schemaVersion: 1, error: { code: 'source_unavailable' } });
  assert.equal(output.content.includes('private-sheet-id'), false);
  assert.equal(output.content.includes(token), false);
  assert.equal(output.content.includes('upstream failed'), false);
});

test('doGet converts sheet read exceptions into source_unavailable', () => {
  const services = makeServices({ rangeError: new Error('service details must remain private') });
  const { payload, output } = callHandler(authorizedEvent(), services);

  assert.equal(payload.error.code, 'source_unavailable');
  assert.equal(output.content.includes('service details'), false);
});

test('doGet returns JSON ContentService output and successful Apps Script data', () => {
  const services = makeServices({ rows: [['symbol', 'price'], ['PTT', 0]] });
  const startedAt = Date.now();
  const { payload, mimeType } = callHandler(authorizedEvent(), services);
  const generatedAt = Date.parse(payload.generatedAt);

  assert.equal(mimeType, 'application/json');
  assert.equal(Number.isFinite(generatedAt), true);
  assert.equal(generatedAt >= startedAt && generatedAt <= Date.now(), true);
  assert.deepEqual(payload, {
    schemaVersion: 1,
    generatedAt: payload.generatedAt,
    items: [{ symbol: 'PTT', label: 'PTT', price: 0, currency: 'THB' }],
  });
});
