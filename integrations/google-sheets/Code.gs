/**
 * Return a normalized stocks response from a header-row-first Sheet table.
 * This function is pure so the payload rules can be exercised without Apps Script.
 */
function buildStocksPayload(rows, now) {
  if (!Array.isArray(rows) || rows.length === 0 || !Array.isArray(rows[0])) {
    return stocksError_('invalid_sheet');
  }

  var recognizedHeaders = ['symbol', 'label', 'price', 'changepercent', 'currency', 'quotedat'];
  var columns = {};
  var header = rows[0];

  for (var headerIndex = 0; headerIndex < header.length; headerIndex += 1) {
    var rawHeader = header[headerIndex];
    if (typeof rawHeader !== 'string') continue;
    var normalizedHeader = rawHeader.trim().toLowerCase();
    if (recognizedHeaders.indexOf(normalizedHeader) === -1) continue;
    if (Object.prototype.hasOwnProperty.call(columns, normalizedHeader)) {
      return stocksError_('invalid_sheet');
    }
    columns[normalizedHeader] = headerIndex;
  }

  if (!Object.prototype.hasOwnProperty.call(columns, 'symbol') ||
      !Object.prototype.hasOwnProperty.call(columns, 'price')) {
    return stocksError_('invalid_sheet');
  }

  var generatedAt;
  try {
    generatedAt = now.toISOString();
  } catch (error) {
    return stocksError_('invalid_sheet');
  }

  var items = [];
  var dataRowCount = Math.min(rows.length - 1, 200);
  for (var rowIndex = 1; rowIndex <= dataRowCount && items.length < 5; rowIndex += 1) {
    var row = rows[rowIndex];
    if (!Array.isArray(row)) return stocksError_('invalid_sheet');

    var rawSymbol = cellAt_(row, columns.symbol);
    if (isBlank_(rawSymbol)) continue;
    if (typeof rawSymbol !== 'string' || hasControlCharacters_(rawSymbol)) {
      return stocksError_('invalid_sheet');
    }
    var symbol = rawSymbol.trim();
    if (symbol.length === 0) continue;
    if (symbol.length > 32) return stocksError_('invalid_sheet');

    var label = symbol;
    if (Object.prototype.hasOwnProperty.call(columns, 'label')) {
      var labelResult = textCell_(cellAt_(row, columns.label), 64);
      if (!labelResult.valid) return stocksError_('invalid_sheet');
      if (!labelResult.blank) label = labelResult.value;
    }

    var currency = 'THB';
    if (Object.prototype.hasOwnProperty.call(columns, 'currency')) {
      var currencyResult = textCell_(cellAt_(row, columns.currency), 8);
      if (!currencyResult.valid) return stocksError_('invalid_sheet');
      if (!currencyResult.blank) currency = currencyResult.value;
    }

    var item = { symbol: symbol, label: label };
    var priceResult = numericCell_(cellAt_(row, columns.price));
    if (!priceResult.valid || (!priceResult.blank && priceResult.value < 0)) {
      return stocksError_('invalid_sheet');
    }
    if (!priceResult.blank) item.price = priceResult.value;

    if (Object.prototype.hasOwnProperty.call(columns, 'changepercent')) {
      var changeResult = numericCell_(cellAt_(row, columns.changepercent));
      if (!changeResult.valid) return stocksError_('invalid_sheet');
      if (!changeResult.blank) item.changePercent = changeResult.value;
    }

    item.currency = currency;
    if (Object.prototype.hasOwnProperty.call(columns, 'quotedat')) {
      var quoteResult = quotedAtCell_(cellAt_(row, columns.quotedat));
      if (!quoteResult.valid) return stocksError_('invalid_sheet');
      if (!quoteResult.blank) item.quotedAt = quoteResult.value;
    }
    items.push(item);
  }

  return { schemaVersion: 1, generatedAt: generatedAt, items: items };
}

/** Apps Script web-app entry point. */
function doGet(e) {
  try {
    var properties = PropertiesService.getScriptProperties();
    var configuredToken = properties.getProperty('ACCESS_TOKEN');
    if (!isConfiguredValue_(configuredToken)) {
      return stocksJsonOutput_(stocksError_('configuration_error'));
    }

    var suppliedToken = e && e.parameter ? e.parameter.accessToken : null;
    if (typeof suppliedToken !== 'string' || suppliedToken.length === 0 || suppliedToken !== configuredToken) {
      return stocksJsonOutput_(stocksError_('auth_required'));
    }

    var spreadsheetId = properties.getProperty('SPREADSHEET_ID');
    var sheetName = properties.getProperty('SHEET_NAME');
    if (!isConfiguredValue_(spreadsheetId) || !isConfiguredValue_(sheetName)) {
      return stocksJsonOutput_(stocksError_('configuration_error'));
    }

    var spreadsheet = SpreadsheetApp.openById(spreadsheetId);
    var sheet = spreadsheet.getSheetByName(sheetName);
    if (!sheet) return stocksJsonOutput_(stocksError_('invalid_sheet'));

    var lastRow = sheet.getLastRow();
    var lastColumn = sheet.getLastColumn();
    if (lastRow < 1 || lastColumn < 1) {
      return stocksJsonOutput_(stocksError_('invalid_sheet'));
    }

    var rows = sheet.getRange(1, 1, Math.min(lastRow, 201), lastColumn).getValues();
    return stocksJsonOutput_(buildStocksPayload(rows, new Date()));
  } catch (error) {
    return stocksJsonOutput_(stocksError_('source_unavailable'));
  }
}

function stocksError_(code) {
  return { schemaVersion: 1, error: { code: code } };
}

function stocksJsonOutput_(payload) {
  return ContentService.createTextOutput(JSON.stringify(payload))
    .setMimeType(ContentService.MimeType.JSON);
}

function isConfiguredValue_(value) {
  return typeof value === 'string' && value.trim().length > 0;
}

function cellAt_(row, columnIndex) {
  if (columnIndex === undefined || columnIndex >= row.length) return '';
  var value = row[columnIndex];
  return value === null || value === undefined ? '' : value;
}

function isBlank_(value) {
  return value === null || value === undefined ||
    (typeof value === 'string' && value.trim().length === 0);
}

function hasControlCharacters_(value) {
  return /[\x00-\x1F\x7F-\x9F]/.test(value);
}

function textCell_(value, maximumLength) {
  if (isBlank_(value)) return { valid: true, blank: true };
  if (typeof value !== 'string' || hasControlCharacters_(value)) {
    return { valid: false, blank: false };
  }
  var normalized = value.trim();
  if (normalized.length === 0) return { valid: true, blank: true };
  if (normalized.length > maximumLength) return { valid: false, blank: false };
  return { valid: true, blank: false, value: normalized };
}

function numericCell_(value) {
  if (isBlank_(value)) return { valid: true, blank: true };
  if (typeof value === 'number') {
    return isFinite(value)
      ? { valid: true, blank: false, value: value }
      : { valid: false, blank: false };
  }
  if (typeof value !== 'string') return { valid: false, blank: false };
  var normalized = value.trim();
  if (normalized.length === 0) return { valid: true, blank: true };
  if (!/^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?$/.test(normalized)) {
    return { valid: false, blank: false };
  }
  var parsed = Number(normalized);
  return isFinite(parsed)
    ? { valid: true, blank: false, value: parsed }
    : { valid: false, blank: false };
}

function quotedAtCell_(value) {
  if (isBlank_(value)) return { valid: true, blank: true };

  if (Object.prototype.toString.call(value) === '[object Date]') {
    var dateMilliseconds = value.getTime();
    if (!isFinite(dateMilliseconds)) return { valid: false, blank: false };
    return { valid: true, blank: false, value: new Date(dateMilliseconds).toISOString() };
  }

  if (typeof value !== 'string') return { valid: false, blank: false };
  var normalized = value.trim();
  if (normalized.length === 0) return { valid: true, blank: true };

  var match = /^(\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2})(?:\.(\d{1,3}))?(Z|[+-]\d{2}:\d{2})$/.exec(normalized);
  if (!match) return { valid: false, blank: false };

  var year = Number(match[1]);
  var month = Number(match[2]);
  var day = Number(match[3]);
  var hour = Number(match[4]);
  var minute = Number(match[5]);
  var second = Number(match[6]);
  var daysInMonth = [31, isLeapYear_(year) ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31];
  if (month < 1 || month > 12 || day < 1 || day > daysInMonth[month - 1] ||
      hour > 23 || minute > 59 || second > 59) {
    return { valid: false, blank: false };
  }

  var zone = match[8];
  if (zone !== 'Z') {
    var offsetHour = Number(zone.slice(1, 3));
    var offsetMinute = Number(zone.slice(4, 6));
    if (offsetMinute > 59 || offsetHour > 14 || (offsetHour === 14 && offsetMinute !== 0)) {
      return { valid: false, blank: false };
    }
  }

  var milliseconds = Date.parse(normalized);
  if (!isFinite(milliseconds)) return { valid: false, blank: false };
  return { valid: true, blank: false, value: new Date(milliseconds).toISOString() };
}

function isLeapYear_(year) {
  return year % 4 === 0 && (year % 100 !== 0 || year % 400 === 0);
}
