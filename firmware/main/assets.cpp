#include "assets.hpp"
namespace deskduo {
// Geometry from the approved Desk Duo mockup; one canvas draw event per mascot.
const char* const sprite_claude[16] = {
"................","................","................","...oooooooooo...",
"..oooooooooooo..",".oooooooooooooo.","oooooooooooooooo","ooooeeooooeeoooo",
"ooooeeooooeeoooo","oooooooooooooooo",".oooooooooooooo.","..oooooooooooo..",
"...oooooooooo...","...oo..oo..oo...","...oo..oo..oo...","................"};
const char* const sprite_codex[16] = {
".......gg.......",".......gg.......","....gggggggg....","...gwwwwwwwwg...",
"...gwggwwggwg...","...gwwwwwwwwg...","...gwwggggwwg...","....gggggggg....",
"..gggggggggggg..","..gwggggggggwg..","..gwggwwwwggwg..","..gggggggggggg..",
"....gggggggg....",".....gg..gg.....","....ggg..ggg....","...www....www..."};
const char portal_html[] = R"HTML(<!doctype html><html lang="th"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>ตั้งค่า Desk Duo</title>
<style>body{font:16px system-ui;max-width:680px;margin:auto;padding:20px;background:#000;color:#e9edf2}fieldset{min-width:0;margin:18px 0;border:1px solid #363a40;border-radius:8px}label{display:block;margin:10px 0}input,textarea,select,button{font:inherit;box-sizing:border-box;max-width:100%}input:not([type=checkbox]),textarea{display:block;width:100%;padding:8px}textarea{height:84px}small{overflow-wrap:anywhere}button{padding:10px 20px}#status,#firmwareStatus{white-space:pre-wrap}input,textarea,select{background:#14171b;color:#e9edf2;border:1px solid #515861;border-radius:6px}button{background:#a5c9ee;color:#061322;border:0;border-radius:8px}button:disabled{opacity:.5}progress{width:100%;accent-color:#a5c9ee}input[type=range]{min-height:44px;accent-color:#c6e7dc;padding:0!important}input[type=file]{position:absolute;opacity:0;width:1px!important;height:1px;padding:0}input[type=file]:focus+label{outline:2px solid #e9edf2;outline-offset:2px}.filepick{display:inline-block;padding:10px 20px;background:#a5c9ee;color:#061322;border-radius:8px;cursor:pointer}</style>
<h1>Desk Duo</h1><p><span data-i18n="ตั้งค่า Wi-Fi และแหล่งข้อมูลของจอ ค่าลับที่เว้นว่างจะคงค่าเดิม">ตั้งค่า Wi-Fi และแหล่งข้อมูลของจอ ค่าลับที่เว้นว่างจะคงค่าเดิม</span></p>
<form id="settings" autocomplete="off">
<fieldset><legend><span data-i18n="Wi-Fi / usage">Wi-Fi / การใช้งาน</span></legend>
<small id="ssidStored"><span data-i18n="SSID ที่บันทึก: กำลังโหลด...">SSID ที่บันทึก: กำลังโหลด...</span></small>
<label><span data-i18n="แก้ไข Wi-Fi SSID (ว่าง = คงค่าเดิม)">แก้ไข Wi-Fi SSID (ว่าง = คงค่าเดิม)</span><input id="ssid" maxlength="32"></label><label><input id="clearSsid" type="checkbox"><span data-i18n=" ปิดการเชื่อมต่อ Wi-Fi"> ปิดการเชื่อมต่อ Wi-Fi</span></label>
<small id="wifiPasswordStored"><span data-i18n="รหัส Wi-Fi ปัจจุบัน: กำลังโหลด...">รหัส Wi-Fi ปัจจุบัน: กำลังโหลด...</span></small>
<label><span data-i18n="เปลี่ยนรหัส Wi-Fi">เปลี่ยนรหัส Wi-Fi</span><input id="wifiPassword" type="password" maxlength="64" autocomplete="new-password"></label>
<label><input id="clearWifi" type="checkbox"><span data-i18n=" ล้างรหัส Wi-Fi (เครือข่ายเปิด)"> ล้างรหัส Wi-Fi (เครือข่ายเปิด)</span></label>
<small id="usageTokenStored"><span data-i18n="Usage token ปัจจุบัน: กำลังโหลด...">โทเค็นส่งข้อมูลปัจจุบัน: กำลังโหลด...</span></small>
<label><span data-i18n="เปลี่ยน Usage bearer token">เปลี่ยนโทเค็นส่งข้อมูล</span><input id="usageToken" type="password" minlength="16" maxlength="128" autocomplete="new-password"></label>
<label><span data-i18n="หมุนหน้าอัตโนมัติ (0 ปิด หรือ 5–300 วินาที)">หมุนหน้าอัตโนมัติ (0 ปิด หรือ 5–300 วินาที)</span><input id="autoRotateSeconds" type="number" min="0" max="300"></label></fieldset>
<div id="sources"></div>
<fieldset><legend><span data-i18n="Touch calibration">ปรับเทียบสัมผัส</span></legend><p><span data-i18n="ตรวจมุมจอจริงก่อนแก้ ค่า JSON ใช้ swapAxes, invertX, invertY, minX, maxX, minY, maxY">ตรวจมุมจอจริงก่อนแก้ ค่า JSON ใช้ swapAxes, invertX, invertY, minX, maxX, minY, maxY</span></p><textarea id="touch" spellcheck="false"></textarea></fieldset>
<button id="settingsButton" type="submit" disabled><span data-i18n="บันทึก">บันทึก</span></button><p id="status" role="status"></p></form>
<form id="display"><fieldset><legend><span data-i18n="การแสดง usage ของ Codex">การแสดงผล / การใช้งาน Codex</span></legend>
<label><span data-i18n="ภาษา / Language">ภาษา</span><select id="language"><option value="th" data-i18n="ไทย / Thai">ไทย</option><option value="en" data-i18n="อังกฤษ / English">อังกฤษ</option></select></label><p id="languageStored" role="status"></p>
<label><span data-i18n="แสดงช่วง usage">แสดงช่วงการใช้งาน</span><select id="codexUsageMode"><option value="both" data-i18n="5h + Weekly">5 ชม. + สัปดาห์</option><option value="weekly" data-i18n="Weekly อย่างเดียว">สัปดาห์อย่างเดียว</option></select></label>
<label><span data-i18n="ความสว่าง">ความสว่าง</span> <output id="brightnessValue" for="brightnessPercent">100%</output><input id="brightnessPercent" type="range" min="5" max="100" step="1" value="100"></label><p id="brightnessStored" role="status"></p>
<label><input id="brightnessAuto" type="checkbox"><span data-i18n="ความสว่างอัตโนมัติ">ความสว่างอัตโนมัติ</span></label>
<label><span data-i18n="เริ่มกลางวัน">เริ่มกลางวัน</span><input id="brightnessDayStart" type="time" step="60" required value="06:00"></label>
<label><span data-i18n="เริ่มกลางคืน">เริ่มกลางคืน</span><input id="brightnessNightStart" type="time" step="60" required value="18:00"></label>
<small data-i18n="ใช้นาฬิกาบน ESP32 กลางวันเต็มค่าฐาน กลางคืนครึ่งหนึ่ง (ต่ำสุด 5%)">ใช้นาฬิกาบน ESP32 กลางวันเต็มค่าฐาน กลางคืนครึ่งหนึ่ง (ต่ำสุด 5%)</small>
<p id="displayCurrent" role="status"></p><p id="brightnessScheduleStored" role="status"></p><p id="brightnessEffective" role="status"></p>
<button id="displayButton" type="submit" disabled><span data-i18n="บันทึกการแสดง usage">บันทึกการแสดงผล</span></button><p id="displayStatus" role="status"></p>
</fieldset></form>
<form id="firmware"><fieldset><legend><span data-i18n="Firmware update">อัปเดตเฟิร์มแวร์</span></legend>
<p id="firmwareInfo"><span data-i18n="Loading firmware status...">กำลังโหลดสถานะเฟิร์มแวร์...</span></p>
<p><span data-i18n="เลือก desk-duo-app.bin สำหรับอัปเดตผ่าน Wi-Fi การตั้งค่าจะคงอยู่">เลือก desk-duo-app.bin สำหรับอัปเดตผ่าน Wi-Fi การตั้งค่าจะคงอยู่</span></p>
<input id="firmwareFile" type="file" accept=".bin,application/octet-stream" required>
<label for="firmwareFile" class="filepick" data-i18n="เลือกไฟล์แอป">เลือกไฟล์แอป</label><p id="firmwareSelected" role="status"></p>
<button id="firmwareButton" type="submit"><span data-i18n="Upload firmware">อัปโหลดเฟิร์มแวร์</span></button>
<progress id="firmwareProgress" max="100" value="0"></progress><p id="firmwareStatus" role="status"></p>
</fieldset></form>
<script>
const $=id=>document.getElementById(id);
const copy={
"ความสว่างอัตโนมัติ":{"th": "ความสว่างอัตโนมัติ", "en": "Auto brightness"},
"เริ่มกลางวัน":{"th": "เริ่มกลางวัน", "en": "Day starts"},
"เริ่มกลางคืน":{"th": "เริ่มกลางคืน", "en": "Night starts"},
"ใช้นาฬิกาบน ESP32 กลางวันเต็มค่าฐาน กลางคืนครึ่งหนึ่ง (ต่ำสุด 5%)":{"th": "ใช้นาฬิกาบน ESP32 กลางวันเต็มค่าฐาน กลางคืนครึ่งหนึ่ง (ต่ำสุด 5%)", "en": "Uses the clock on ESP32. Day uses the full base; night uses half (minimum 5%)."},
"อัตโนมัติที่บันทึก: ":{"th": "อัตโนมัติที่บันทึก: ", "en": "Saved Auto: "},
"เปิด":{"th": "เปิด", "en": "On"},
"ปิด":{"th": "ปิด", "en": "Off"},
" · กลางวัน ":{"th": " · กลางวัน ", "en": " · Day "},
" · กลางคืน ":{"th": " · กลางคืน ", "en": " · Night "},
"ยังไม่ทราบเวลาที่บันทึก":{"th": "ยังไม่ทราบเวลาที่บันทึก", "en": "Saved Auto and times unknown"},
"เป้าหมาย ":{"th": "เป้าหมาย ", "en": "Target "},
" · ใช้ล่าสุด ":{"th": " · ใช้ล่าสุด ", "en": " · Last applied "},
"ปรับเอง":{"th": "ปรับเอง", "en": "Manual"},
"กลางวัน":{"th": "กลางวัน", "en": "Day"},
"กลางคืน":{"th": "กลางคืน", "en": "Night"},
"รอนาฬิกา":{"th": "รอนาฬิกา", "en": "Waiting for clock"},
"สถานะแสง: ไม่ทราบ":{"th": "สถานะแสง: ไม่ทราบ", "en": "Brightness status: Unknown"},
"เวลาต้องเป็น HH:MM และไม่ซ้ำกัน":{"th": "เวลาต้องเป็น HH:MM และไม่ซ้ำกัน", "en": "Times must be distinct HH:MM values"},
"โหมดที่บันทึก: ":{"th": "โหมดที่บันทึก: ", "en": "Saved mode: "},

 "ความสว่าง":{"th":"ความสว่างฐาน","en":"Base brightness"},
 "ความสว่างที่บันทึก: ":{"th":"ความสว่างที่บันทึก: ","en":"Saved brightness: "},
 "ยังไม่ทราบความสว่างที่บันทึก":{"th":"ยังไม่ทราบความสว่างที่บันทึก","en":"Saved brightness unknown"},
 "Desk Duo setup": {
  "th": "ตั้งค่า Desk Duo",
  "en": "Desk Duo setup"
 },
 "ตั้งค่า Wi-Fi และแหล่งข้อมูลของจอ ค่าลับที่เว้นว่างจะคงค่าเดิม": {
  "th": "ตั้งค่า Wi-Fi และแหล่งข้อมูลของจอ ค่าลับที่เว้นว่างจะคงค่าเดิม",
  "en": "Configure Wi-Fi and screen data sources. Blank secrets keep their saved values."
 },
 "Wi-Fi / usage": {
  "th": "Wi-Fi / การใช้งาน",
  "en": "Wi-Fi / usage"
 },
 "SSID ที่บันทึก: กำลังโหลด...": {
  "th": "SSID ที่บันทึก: กำลังโหลด...",
  "en": "Saved SSID: loading..."
 },
 "แก้ไข Wi-Fi SSID (ว่าง = คงค่าเดิม)": {
  "th": "แก้ไข Wi-Fi SSID (ว่าง = คงค่าเดิม)",
  "en": "Edit Wi-Fi SSID (blank = keep saved)"
 },
 " ปิดการเชื่อมต่อ Wi-Fi": {
  "th": " ปิดการเชื่อมต่อ Wi-Fi",
  "en": " Disconnect Wi-Fi"
 },
 "รหัส Wi-Fi ปัจจุบัน: กำลังโหลด...": {
  "th": "รหัส Wi-Fi ปัจจุบัน: กำลังโหลด...",
  "en": "Wi-Fi password: loading..."
 },
 "เปลี่ยนรหัส Wi-Fi": {
  "th": "เปลี่ยนรหัส Wi-Fi",
  "en": "Replace Wi-Fi password"
 },
 " ล้างรหัส Wi-Fi (เครือข่ายเปิด)": {
  "th": " ล้างรหัส Wi-Fi (เครือข่ายเปิด)",
  "en": " Clear Wi-Fi password (open network)"
 },
 "Usage token ปัจจุบัน: กำลังโหลด...": {
  "th": "โทเค็นส่งข้อมูลปัจจุบัน: กำลังโหลด...",
  "en": "Usage token: loading..."
 },
 "เปลี่ยน Usage bearer token": {
  "th": "เปลี่ยนโทเค็นส่งข้อมูล",
  "en": "Replace usage bearer token"
 },
 "หมุนหน้าอัตโนมัติ (0 ปิด หรือ 5–300 วินาที)": {
  "th": "หมุนหน้าอัตโนมัติ (0 ปิด หรือ 5–300 วินาที)",
  "en": "Auto rotation (0 off or 5–300 seconds)"
 },
 "Touch calibration": {
  "th": "ปรับเทียบสัมผัส",
  "en": "Touch calibration"
 },
 "ตรวจมุมจอจริงก่อนแก้ ค่า JSON ใช้ swapAxes, invertX, invertY, minX, maxX, minY, maxY": {
  "th": "ตรวจมุมจอจริงก่อนแก้ ค่า JSON ใช้ swapAxes, invertX, invertY, minX, maxX, minY, maxY",
  "en": "Check the physical screen corners before editing. JSON keys: swapAxes, invertX, invertY, minX, maxX, minY, maxY"
 },
 "บันทึก": {
  "th": "บันทึก",
  "en": "Save settings"
 },
 "การแสดง usage ของ Codex": {
  "th": "การแสดงผล / การใช้งาน Codex",
  "en": "Display / Codex usage"
 },
 "แสดงช่วง usage": {
  "th": "แสดงช่วงการใช้งาน",
  "en": "Usage windows"
 },
 "5h + Weekly": {
  "th": "5 ชม. + สัปดาห์",
  "en": "5h + Weekly"
 },
 "Weekly อย่างเดียว": {
  "th": "สัปดาห์อย่างเดียว",
  "en": "Weekly only"
 },
 "บันทึกการแสดง usage": {
  "th": "บันทึกการแสดงผล",
  "en": "Save display settings"
 },
 "Firmware update": {
  "th": "อัปเดตเฟิร์มแวร์",
  "en": "Firmware update"
 },
 "Loading firmware status...": {
  "th": "กำลังโหลดสถานะเฟิร์มแวร์...",
  "en": "Loading firmware status..."
 },
 "เลือก desk-duo-app.bin สำหรับอัปเดตผ่าน Wi-Fi การตั้งค่าจะคงอยู่": {
  "th": "เลือก desk-duo-app.bin สำหรับอัปเดตผ่าน Wi-Fi การตั้งค่าจะคงอยู่",
  "en": "Choose desk-duo-app.bin to update over Wi-Fi. Settings are preserved."
 },
 "Upload firmware": {
  "th": "อัปโหลดเฟิร์มแวร์",
  "en": "Upload firmware"
 },
 "ภาษา / Language": {
  "th": "ภาษา",
  "en": "Language"
 },
 "ไทย / Thai": {
  "th": "ไทย",
  "en": "Thai"
 },
 "อังกฤษ / English": {
  "th": "อังกฤษ",
  "en": "English"
 },
 "อากาศ": {
  "th": "อากาศ",
  "en": "Weather"
 },
 "หุ้นจาก Google Sheets": {
  "th": "หุ้นจาก Google Sheets",
  "en": "Stocks from Google Sheets"
 },
 "URL ที่บันทึก: กำลังโหลด...": {
  "th": "URL ที่บันทึก: กำลังโหลด...",
  "en": "Saved URL: loading..."
 },
 "แทนที่ URL (ว่าง = คงค่าเดิม)": {
  "th": "แทนที่ URL (ว่าง = คงค่าเดิม)",
  "en": "Replace URL (blank = keep saved)"
 },
 " ปิดแหล่งข้อมูล / ล้าง URL": {
  "th": " ปิดแหล่งข้อมูล / ล้าง URL",
  "en": " Disable source / clear URL"
 },
 "Headers ปัจจุบัน: กำลังโหลด...": {
  "th": "ส่วนหัวปัจจุบัน: กำลังโหลด...",
  "en": "Current headers: loading..."
 },
 "แทนที่ headers เป็น JSON (ว่าง = คงค่าเดิม, {} = ล้าง)": {
  "th": "แทนที่ส่วนหัวเป็น JSON (ว่าง = คงค่าเดิม, {} = ล้าง)",
  "en": "Replace headers as JSON (blank = keep saved, {} = clear)"
 },
 "Refresh วินาที (30–86400)": {
  "th": "ดึงข้อมูลทุกกี่วินาที (30–86400)",
  "en": "Refresh seconds (30–86400)"
 },
 "ข้อมูลอากาศ: กำลังโหลด...": {
  "th": "ข้อมูลอากาศ: กำลังโหลด...",
  "en": "Weather data: loading..."
 },
 "อัปเดตสถานะ": {
  "th": "อัปเดตสถานะ",
  "en": "Refresh status"
 },
 "อ่านสถานะล่าสุดของเครื่อง การอ่านไม่สั่งดึงข้อมูลใหม่": {
  "th": "อ่านสถานะล่าสุดของเครื่อง การอ่านไม่สั่งดึงข้อมูลใหม่",
  "en": "Read the latest device status. This does not fetch new source data."
 },
 "ชื่อสถานที่": {
  "th": "ชื่อสถานที่",
  "en": "Location label"
 },
 "Field mappings (JSON paths)": {
  "th": "จับคู่ฟิลด์ (JSON paths)",
  "en": "Field mappings (JSON paths)"
 },
 "ตั้งค่าแล้ว": {
  "th": "ตั้งค่าแล้ว",
  "en": "Configured"
 },
 "ยังไม่ตั้งค่า": {
  "th": "ยังไม่ตั้งค่า",
  "en": "Not configured"
 },
 "ไม่ทราบ": {
  "th": "ไม่ทราบ",
  "en": "Unknown"
 },
 "ค่าปัจจุบัน: ไม่ทราบ (โหลดไม่สำเร็จ)": {
  "th": "ค่าปัจจุบัน: ไม่ทราบ (โหลดไม่สำเร็จ)",
  "en": "Current value: unknown (load failed)"
 },
 "โหลดการตั้งค่าไม่สำเร็จ": {
  "th": "โหลดการตั้งค่าไม่สำเร็จ",
  "en": "Could not load settings"
 },
 "SSID ที่บันทึก: ": {
  "th": "SSID ที่บันทึก: ",
  "en": "Saved SSID: "
 },
 "รหัส Wi-Fi ปัจจุบัน: ": {
  "th": "รหัส Wi-Fi ปัจจุบัน: ",
  "en": "Current Wi-Fi password: "
 },
 "Usage token ปัจจุบัน: ": {
  "th": "โทเค็นส่งข้อมูลปัจจุบัน: ",
  "en": "Current usage token: "
 },
 "URL ที่บันทึก: ": {
  "th": "URL ที่บันทึก: ",
  "en": "Saved URL: "
 },
 " (ซ่อน query / fragment)": {
  "th": " (ซ่อน query / fragment)",
  "en": " (query / fragment hidden)"
 },
 "Headers ปัจจุบัน: ": {
  "th": "ส่วนหัวปัจจุบัน: ",
  "en": "Current headers: "
 },
 "ตรวจสอบค่า JSON object": {
  "th": "ตรวจสอบค่า JSON object",
  "en": "Check the JSON object"
 },
 "รอโหลดการตั้งค่า / โหลดหน้าใหม่ก่อนบันทึก": {
  "th": "รอโหลดการตั้งค่า / โหลดหน้าใหม่ก่อนบันทึก",
  "en": "Wait for settings to load / reload before saving"
 },
 "ไม่มีการเปลี่ยนแปลง": {
  "th": "ไม่มีการเปลี่ยนแปลง",
  "en": "No changes"
 },
 "กำลังบันทึก...": {
  "th": "กำลังบันทึก...",
  "en": "Saving..."
 },
 "Firmware maintenance busy หรือบันทึกไม่สำเร็จ ยังไม่เปลี่ยนค่า กรุณาลองอีกครั้ง": {
  "th": "กำลังบำรุงรักษาหรือบันทึกไม่สำเร็จ ยังไม่เปลี่ยนค่า กรุณาลองอีกครั้ง",
  "en": "Maintenance busy or save failed. Values are unchanged; retry."
 },
 "ตรวจสอบค่า / สิทธิ์ (HTTP ": {
  "th": "ตรวจสอบค่า / สิทธิ์ (HTTP ",
  "en": "Check values / permissions (HTTP "
 },
 "บันทึกแล้ว แต่โหลดค่าปัจจุบันไม่สำเร็จ กรุณาโหลดหน้าใหม่ก่อนแก้ไขอีกครั้ง": {
  "th": "บันทึกแล้ว แต่โหลดค่าปัจจุบันไม่สำเร็จ กรุณาโหลดหน้าใหม่ก่อนแก้ไขอีกครั้ง",
  "en": "Saved, but current settings could not be loaded. Reload before editing again."
 },
 "บันทึกแล้ว Wi-Fi จะเชื่อมต่อใหม่เมื่อเปลี่ยนค่า": {
  "th": "บันทึกแล้ว Wi-Fi จะเชื่อมต่อใหม่เมื่อเปลี่ยนค่า",
  "en": "Saved. Wi-Fi reconnects when its settings change."
 },
 "โหลดการแสดง usage ไม่สำเร็จ": {
  "th": "โหลดการแสดง usage ไม่สำเร็จ",
  "en": "Could not load display settings"
 },
 "รอโหลดการแสดง usage": {
  "th": "รอโหลดการแสดง usage",
  "en": "Wait for display settings to load"
 },
 "ยังบันทึกไม่ได้ กรุณาลองอีกครั้ง": {
  "th": "ยังบันทึกไม่ได้ กรุณาลองอีกครั้ง",
  "en": "Could not save; please retry"
 },
 "บันทึกการแสดง usage แล้ว": {
  "th": "บันทึกการแสดงผลแล้ว",
  "en": "Display settings saved"
 },
 "ยังไม่ตั้งค่าแหล่งข้อมูล": {
  "th": "ยังไม่ตั้งค่าแหล่งข้อมูล",
  "en": "Source not configured"
 },
 "เครือข่ายยังไม่เชื่อมต่อ": {
  "th": "เครือข่ายยังไม่เชื่อมต่อ",
  "en": "Network offline"
 },
 "รอดึงข้อมูลสำหรับการตั้งค่าปัจจุบัน": {
  "th": "รอดึงข้อมูลสำหรับการตั้งค่าปัจจุบัน",
  "en": "Waiting for data with current settings"
 },
 "รอดึงข้อมูล": {
  "th": "รอดึงข้อมูล",
  "en": "Waiting for data"
 },
 "รอเวลาของเครื่องก่อนเชื่อมต่อ HTTPS": {
  "th": "รอเวลาของเครื่องก่อนเชื่อมต่อ HTTPS",
  "en": "Waiting for device time before HTTPS"
 },
 "รับข้อมูลแล้ว": {
  "th": "รับข้อมูลแล้ว",
  "en": "Data received"
 },
 "รูปแบบข้อมูลไม่ถูกต้อง": {
  "th": "รูปแบบข้อมูลไม่ถูกต้อง",
  "en": "Invalid data format"
 },
 "แหล่งข้อมูลตอบ HTTP ": {
  "th": "แหล่งข้อมูลตอบ HTTP ",
  "en": "Source returned HTTP "
 },
 "เชื่อมต่อ HTTPS ไม่สำเร็จ": {
  "th": "เชื่อมต่อ HTTPS ไม่สำเร็จ",
  "en": "HTTPS connection failed"
 },
 "เครือข่าย / การดึงข้อมูลไม่สำเร็จ": {
  "th": "เครือข่าย / การดึงข้อมูลไม่สำเร็จ",
  "en": "Network / fetch failed"
 },
 "รหัสอากาศ ": {
  "th": "รหัสอากาศ ",
  "en": "Weather code "
 },
 "รายชั่วโมง ": {
  "th": "รายชั่วโมง ",
  "en": "Hourly "
 },
 "อายุ ": {
  "th": "อายุ ",
  "en": "Age "
 },
 " วินาที": {
  "th": " วินาที",
  "en": " seconds"
 },
 "ข้อมูลอากาศ: ": {
  "th": "ข้อมูลอากาศ: ",
  "en": "Weather data: "
 },
 "ข้อมูลเก่า": {
  "th": "ข้อมูลเก่า",
  "en": "Stale data"
 },
 "มีข้อมูล": {
  "th": "มีข้อมูล",
  "en": "Data available"
 },
 "ยังไม่มีข้อมูล": {
  "th": "ยังไม่มีข้อมูล",
  "en": "No data"
 },
 "ข้อมูลอากาศ: รออ่านสถานะสำหรับการตั้งค่าปัจจุบัน": {
  "th": "ข้อมูลอากาศ: รออ่านสถานะสำหรับการตั้งค่าปัจจุบัน",
  "en": "Weather data: waiting for status with current settings"
 },
 "ข้อมูลอากาศ: ไม่ทราบ (อ่านสถานะไม่สำเร็จ กรุณาลองอีกครั้ง)": {
  "th": "ข้อมูลอากาศ: ไม่ทราบ (อ่านสถานะไม่สำเร็จ กรุณาลองอีกครั้ง)",
  "en": "Weather data: unknown (status read failed; retry)"
 },
 "ภาษาที่บันทึก: ": {
  "th": "ภาษาที่บันทึก: ",
  "en": "Saved language: "
 },
 "ยังไม่ทราบภาษาที่บันทึก": {
  "th": "ยังไม่ทราบภาษาที่บันทึก",
  "en": "Saved language unknown"
 },
 " · ยังไม่ได้บันทึก": {
  "th": " · ยังไม่ได้บันทึก",
  "en": " · Not saved"
 },
 "บันทึกแล้ว แต่โหลดการแสดงผลไม่สำเร็จ กรุณาโหลดหน้าใหม่": {
  "th": "บันทึกแล้ว แต่โหลดการแสดงผลไม่สำเร็จ กรุณาโหลดหน้าใหม่",
  "en": "Saved, but display readback failed. Please reload."
 },
 "Firmware status unavailable (HTTP ": {
  "th": "ไม่สามารถอ่านสถานะเฟิร์มแวร์ (HTTP ",
  "en": "Firmware status unavailable (HTTP "
 },
 "Version ": {
  "th": "รุ่น ",
  "en": "Version "
 },
 " bytes capacity": {
  "th": " ไบต์ความจุ",
  "en": " bytes capacity"
 },
 " · Update busy": {
  "th": " · กำลังอัปเดต",
  "en": " · Update busy"
 },
 "Validated. Restarting device; waiting to reconnect...": {
  "th": "ตรวจสอบแล้ว กำลังเริ่มเครื่องใหม่ รอเชื่อมต่อ...",
  "en": "Validated. Restarting device; waiting to reconnect..."
 },
 "Update complete. Connected to version ": {
  "th": "อัปเดตเสร็จแล้ว เชื่อมต่อรุ่น ",
  "en": "Update complete. Connected to version "
 },
 "Reconnect could not be confirmed. Check the device and reload this page to retry.": {
  "th": "ยืนยันการเชื่อมต่อไม่ได้ ตรวจเครื่องและโหลดหน้านี้ใหม่เพื่อลองอีกครั้ง",
  "en": "Reconnect could not be confirmed. Check the device and reload this page to retry."
 },
 "Choose an application .bin (desk-duo-app.bin).": {
  "th": "เลือกไฟล์แอป .bin (desk-duo-app.bin)",
  "en": "Choose an application .bin (desk-duo-app.bin)."
 },
 "Firmware maintenance busy. Retry shortly.": {
  "th": "กำลังบำรุงรักษาเฟิร์มแวร์ กรุณาลองอีกครั้ง",
  "en": "Firmware maintenance busy. Retry shortly."
 },
 "Application size exceeds slot capacity or file is empty.": {
  "th": "ไฟล์แอปเกินความจุหรือเป็นไฟล์ว่าง",
  "en": "Application size exceeds slot capacity or file is empty."
 },
 "Uploading... keep the device powered.": {
  "th": "กำลังอัปโหลด... ให้เครื่องเปิดอยู่",
  "en": "Uploading... keep the device powered."
 },
 "Upload sent. Verifying application...": {
  "th": "ส่งไฟล์แล้ว กำลังตรวจสอบแอป...",
  "en": "Upload sent. Verifying application..."
 },
 "Upload interrupted. Check device status and retry.": {
  "th": "การอัปโหลดขาดตอน ตรวจสถานะเครื่องและลองอีกครั้ง",
  "en": "Upload interrupted. Check device status and retry."
 },
 "Update rejected (HTTP ": {
  "th": "การอัปเดตถูกปฏิเสธ (HTTP ",
  "en": "Update rejected (HTTP "
 },
 "). Check the application file and retry.": {
  "th": "). ตรวจไฟล์แอปและลองอีกครั้ง",
  "en": "). Check the application file and retry."
 },
 "เลือกไฟล์แอป": {
  "th": "เลือกไฟล์แอป",
  "en": "Choose application file"
 },
 "ยังไม่เลือกไฟล์": {
  "th": "ยังไม่เลือกไฟล์",
  "en": "No file selected"
 }
};
let language='th';
const t=key=>copy[key]?.[language]??key;
const messages=new Map();
function put(id,render){messages.set(id,render);$(id).textContent=render();}
function applyLanguage(value){
 language=value==='en'?'en':'th';document.documentElement.lang=language;document.title=t('Desk Duo setup');
 for(const node of document.querySelectorAll('[data-i18n]'))node.textContent=t(node.dataset.i18n);
 for(const [id,render] of messages)$(id).textContent=render();
}
function issue(render){const error=new Error();error.render=render;return error;}
function errorText(error){return error.render?error.render():t("ยังบันทึกไม่ได้ กรุณาลองอีกครั้ง");}
function parseJson(text){try{return JSON.parse(text);}catch(_){throw issue(()=>t("ตรวจสอบค่า JSON object"));}}

for(const name of ['weather','stocks']){
 const box=document.createElement('fieldset');
 box.innerHTML=`<legend><span data-i18n="${name==='weather'?'อากาศ':'หุ้นจาก Google Sheets'}"></span></legend>
 <small id="${name}Stored"><span data-i18n="URL ที่บันทึก: กำลังโหลด...">URL ที่บันทึก: กำลังโหลด...</span></small>
 <label><span data-i18n="แทนที่ URL (ว่าง = คงค่าเดิม)">แทนที่ URL (ว่าง = คงค่าเดิม)</span><input id="${name}Url" type="url" maxlength="1024" placeholder="https://..."></label>
 <label><input id="${name}Clear" type="checkbox"><span data-i18n=" ปิดแหล่งข้อมูล / ล้าง URL"> ปิดแหล่งข้อมูล / ล้าง URL</span></label>
 <small id="${name}HeadersStored"><span data-i18n="Headers ปัจจุบัน: กำลังโหลด...">ส่วนหัวปัจจุบัน: กำลังโหลด...</span></small>
 <label><span data-i18n="แทนที่ headers เป็น JSON (ว่าง = คงค่าเดิม, {} = ล้าง)">แทนที่ส่วนหัวเป็น JSON (ว่าง = คงค่าเดิม, {} = ล้าง)</span><textarea id="${name}Headers" spellcheck="false" placeholder='{"Authorization":"Bearer ..."}'></textarea></label>
 <label><span data-i18n="Refresh วินาที (30–86400)">ดึงข้อมูลทุกกี่วินาที (30–86400)</span><input id="${name}Refresh" type="number" min="30" max="86400"></label>`;
 if(name==='weather')box.innerHTML+='<p id="weatherDataStatus" role="status"><span data-i18n="ข้อมูลอากาศ: กำลังโหลด...">ข้อมูลอากาศ: กำลังโหลด...</span></p><button id="networkRefresh" type="button"><span data-i18n="อัปเดตสถานะ">อัปเดตสถานะ</span></button><small><span data-i18n="อ่านสถานะล่าสุดของเครื่อง การอ่านไม่สั่งดึงข้อมูลใหม่">อ่านสถานะล่าสุดของเครื่อง การอ่านไม่สั่งดึงข้อมูลใหม่</span></small><label><span data-i18n="ชื่อสถานที่">ชื่อสถานที่</span><input id="weatherLabel" maxlength="64"></label><label><span data-i18n="Field mappings (JSON paths)">จับคู่ฟิลด์ (JSON paths)</span><textarea id="weatherFields" spellcheck="false"></textarea></label>';
 $('sources').append(box);
}
applyLanguage('th');
let saved;
let saving=false;
const presence=value=>value===true?t("ตั้งค่าแล้ว"):value===false?t("ยังไม่ตั้งค่า"):t("ไม่ทราบ");
function unknownSettings(){
 saved=undefined;$('settingsButton').disabled=true;
 for(const id of ['ssidStored','wifiPasswordStored','usageTokenStored','weatherStored','stocksStored','weatherHeadersStored','stocksHeadersStored'])put(id,()=>t("ค่าปัจจุบัน: ไม่ทราบ (โหลดไม่สำเร็จ)"));
}
async function load(){
 saved=undefined;$('settingsButton').disabled=true;
 try{
  const r=await fetch('/api/config',{cache:'no-store'});if(!r.ok)throw issue(()=>t("โหลดการตั้งค่าไม่สำเร็จ"));
  const current=await r.json();
  // Do not permit writes from an incomplete or older redacted response.
  const object=value=>value && typeof value==='object' && !Array.isArray(value);
  if(typeof current.currentSSID!=='string' || typeof current.wifiPasswordConfigured!=='boolean' || typeof current.usageTokenConfigured!=='boolean' || !Number.isFinite(current.autoRotateSeconds) ||
     !object(current.touch) || !['swapAxes','invertX','invertY'].every(key=>typeof current.touch[key]==='boolean') || !['minX','maxX','minY','maxY'].every(key=>Number.isFinite(current.touch[key])) ||
     !object(current.weather?.fields) || !Object.values(current.weather.fields).every(value=>typeof value==='string') || typeof current.weather.label!=='string' ||
     !['weather','stocks'].every(n=>object(current[n]) && typeof current[n].url==='string' && Number.isFinite(current[n].refreshSeconds) && typeof current[n].headersConfigured==='boolean' && typeof current[n].urlDetailsHidden==='boolean'))throw issue(()=>t("โหลดการตั้งค่าไม่สำเร็จ"));
  $('ssid').value=current.currentSSID;put('ssidStored',()=>t("SSID ที่บันทึก: ")+(current.currentSSID||t("ยังไม่ตั้งค่า")));
  put('wifiPasswordStored',()=>t("รหัส Wi-Fi ปัจจุบัน: ")+presence(current.wifiPasswordConfigured));
  put('usageTokenStored',()=>t("Usage token ปัจจุบัน: ")+presence(current.usageTokenConfigured));
  $('clearSsid').checked=false;$('autoRotateSeconds').value=current.autoRotateSeconds;$('touch').value=JSON.stringify(current.touch,null,2);
  for(const n of ['weather','stocks']){
   put(n+'Stored',()=>t("URL ที่บันทึก: ")+(current[n].url||t("ยังไม่ตั้งค่า"))+(current[n].urlDetailsHidden?t(" (ซ่อน query / fragment)"):''));
   put(n+'HeadersStored',()=>t("Headers ปัจจุบัน: ")+presence(current[n].headersConfigured));
   $(n+'Url').value='';$(n+'Headers').value='';$(n+'Clear').checked=false;$(n+'Refresh').value=current[n].refreshSeconds;
  }
  $('weatherLabel').value=current.weather.label;$('weatherFields').value=JSON.stringify(current.weather.fields,null,2);
  $('wifiPassword').value='';$('usageToken').value='';$('clearWifi').checked=false;
  saved=current;$('settingsButton').disabled=false;
 }catch(_){unknownSettings();throw issue(()=>t("โหลดการตั้งค่าไม่สำเร็จ"));}
}
function editedObject(text,current){
 let value;try{value=JSON.parse(text);}catch(_){throw issue(()=>t("ตรวจสอบค่า JSON object"));}
 if(!value || typeof value!=='object' || Array.isArray(value))throw issue(()=>t("ตรวจสอบค่า JSON object"));
 return Object.fromEntries(Object.entries(value).filter(([key,item])=>item!==current[key]));
}
$('settings').addEventListener('submit',async e=>{
 e.preventDefault();if(saving)return;
 try{
  if(!saved)throw issue(()=>t("รอโหลดการตั้งค่า / โหลดหน้าใหม่ก่อนบันทึก"));
  const patch={};
  const rotate=Number($('autoRotateSeconds').value);if(rotate!==saved.autoRotateSeconds)patch.autoRotateSeconds=rotate;
  const touch=editedObject($('touch').value,saved.touch);if(Object.keys(touch).length)patch.touch=touch;
  if($('clearSsid').checked)patch.ssid='';else if($('ssid').value && $('ssid').value!==saved.currentSSID)patch.ssid=$('ssid').value;
  if($('clearWifi').checked)patch.wifiPassword='';else if($('wifiPassword').value)patch.wifiPassword=$('wifiPassword').value;
  if($('usageToken').value)patch.usageToken=$('usageToken').value;
  for(const n of ['weather','stocks']){
   const source={};const refresh=Number($(n+'Refresh').value);if(refresh!==saved[n].refreshSeconds)source.refreshSeconds=refresh;
   // Never round-trip GET's redacted URL or any hidden secret.
   if($(n+'Clear').checked)source.url='';else if($(n+'Url').value)source.url=$(n+'Url').value;
   if($(n+'Headers').value.trim())source.headers=parseJson($(n+'Headers').value);
   if(n==='weather'){
    if($('weatherLabel').value!==saved.weather.label)source.label=$('weatherLabel').value;
    const fields=editedObject($('weatherFields').value,saved.weather.fields);if(Object.keys(fields).length)source.fields=fields;
   }
   if(Object.keys(source).length)patch[n]=source;
  }
  if(!Object.keys(patch).length){put('status',()=>t("ไม่มีการเปลี่ยนแปลง"));return;}
  saving=true;$('settingsButton').disabled=true;put('status',()=>t("กำลังบันทึก..."));
  const r=await fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(patch)});
  if(!r.ok)throw issue(()=>r.status===503?t("Firmware maintenance busy หรือบันทึกไม่สำเร็จ ยังไม่เปลี่ยนค่า กรุณาลองอีกครั้ง"):t("ตรวจสอบค่า / สิทธิ์ (HTTP ")+r.status+')');
  invalidateNetworkStatus();
  // The POST has committed even if fetching its new snapshot fails.
  try{await load();}catch(_){put('status',()=>t("บันทึกแล้ว แต่โหลดค่าปัจจุบันไม่สำเร็จ กรุณาโหลดหน้าใหม่ก่อนแก้ไขอีกครั้ง"));return;}
  put('status',()=>t("บันทึกแล้ว Wi-Fi จะเชื่อมต่อใหม่เมื่อเปลี่ยนค่า"));
 }catch(err){put('status',()=>errorText(err));}
 finally{saving=false;$('settingsButton').disabled=!saved;}
});load().catch(e=>put('status',()=>errorText(e)));
let displayLoaded=false,displaySaved,displaySaving=false,displayInitialized=false;
let displayEpoch=0,displayRequest,displayStatusKnown=false;
// Editor reconciliation survives invalidated public saved claims. It never
// establishes saved authority; only a valid actual GET can do that.
let displayReconcile;
const displayKeys=['codexUsageMode','language','brightnessPercent','brightnessAuto','brightnessDayStart','brightnessNightStart'];
const editorValue=key=>key==='brightnessAuto'?$(key).checked:key==='brightnessPercent'?Number($(key).value):$(key).value;
const validTime=value=>typeof value==='string' && /^(?:[01][0-9]|2[0-3]):[0-5][0-9]$/.test(value);
$('displayButton').disabled=true;
function validDisplay(current){
 return current && ['both','weekly'].includes(current.codexUsageMode) && ['th','en'].includes(current.language) && Number.isInteger(current.brightnessPercent) && current.brightnessPercent>=5 && current.brightnessPercent<=100 &&
  typeof current.brightnessAuto==='boolean' && validTime(current.brightnessDayStart) && validTime(current.brightnessNightStart) && current.brightnessDayStart!==current.brightnessNightStart;
}
function displayReadback(current,baseline){
 if(!validDisplay(current))throw issue(()=>t("โหลดการแสดง usage ไม่สำเร็จ"));
 for(const key of displayKeys){
  // A refresh can update a clean editor only. Recheck after awaiting GET so
  // edits made while the request was in flight also survive.
  if(!displayInitialized || (baseline && editorValue(key)===baseline[key])){
   if(key==='brightnessAuto')$(key).checked=current[key];else $(key).value=current[key];
  }
 }
 displaySaved=current;displayReconcile=current;displayInitialized=true;displayLoaded=true;displayStatusKnown=true;
 $('displayButton').disabled=displaySaving;applyLanguage($('language').value);languageReadback();
}
function languageReadback(){
 put('brightnessValue',()=>$('brightnessPercent').value+'%');
 put('brightnessStored',()=>displaySaved?t("ความสว่างที่บันทึก: ")+displaySaved.brightnessPercent+'%'+(Number($('brightnessPercent').value)!==displaySaved.brightnessPercent?t(" · ยังไม่ได้บันทึก"):""):t("ยังไม่ทราบความสว่างที่บันทึก"));
 put('languageStored',()=>displaySaved?t("ภาษาที่บันทึก: ")+t(displaySaved.language==='en'?"อังกฤษ / English":"ไทย / Thai")+($('language').value!==displaySaved.language?t(" · ยังไม่ได้บันทึก"):""):t("ยังไม่ทราบภาษาที่บันทึก"));
 put('displayCurrent',()=>displaySaved?t('โหมดที่บันทึก: ')+t(displaySaved.codexUsageMode==='both'?'5h + Weekly':'Weekly อย่างเดียว'):t('ค่าปัจจุบัน: ไม่ทราบ (โหลดไม่สำเร็จ)'));
 put('brightnessScheduleStored',()=>displaySaved?t('อัตโนมัติที่บันทึก: ')+t(displaySaved.brightnessAuto?'เปิด':'ปิด')+t(' · กลางวัน ')+displaySaved.brightnessDayStart+t(' · กลางคืน ')+displaySaved.brightnessNightStart+
  (['brightnessAuto','brightnessDayStart','brightnessNightStart'].some(key=>editorValue(key)!==displaySaved[key])?t(' · ยังไม่ได้บันทึก'):''):t('ยังไม่ทราบเวลาที่บันทึก'));
 put('brightnessEffective',()=>{
  const state=displaySaved;
  if(!state || !displayStatusKnown)return t('สถานะแสง: ไม่ทราบ');
  const percent=value=>Number.isInteger(value)&&value>=5&&value<=100;
  const phases={manual:'ปรับเอง',day:'กลางวัน',night:'กลางคืน',clock_wait:'รอนาฬิกา'};
  return t('เป้าหมาย ')+(percent(state.brightnessTargetPercent)?state.brightnessTargetPercent+'%':t('ไม่ทราบ'))+t(' · ใช้ล่าสุด ')+(percent(state.brightnessEffectivePercent)?state.brightnessEffectivePercent+'%':t('ไม่ทราบ'))+' · '+(Object.hasOwn(phases,state.brightnessPhase)?t(phases[state.brightnessPhase]):t('ไม่ทราบ'));
 });
}
async function loadDisplay(){
 // Serialize GETs, including the authoritative read after POST. An older GET
 // is allowed to finish but loses publication authority when a save starts.
 if(displayRequest){try{await displayRequest;}catch(_){} }
 if(displayRequest)return loadDisplay();
 const epoch=displayEpoch,baseline=displayReconcile;
 const controller=new AbortController();const timeout=setTimeout(()=>controller.abort(),4000);
 const request=(async()=>{
  const r=await fetch('/api/display',{cache:'no-store',signal:controller.signal});
  if(!r.ok)throw issue(()=>t('โหลดการแสดง usage ไม่สำเร็จ'));
  const current=await r.json();if(epoch!==displayEpoch)return;
  displayReadback(current,baseline);
 })();
 displayRequest=request;
 try{await request;}finally{clearTimeout(timeout);if(displayRequest===request)displayRequest=undefined;}
}
async function refreshDisplay(){
 if(displaySaving || displayRequest || document.hidden)return;
 const epoch=displayEpoch;
 try{await loadDisplay();if(epoch===displayEpoch && !displaySaving)put('displayStatus',()=>displayLoaded?'':t('โหลดการแสดง usage ไม่สำเร็จ'));}
 catch(_){if(epoch===displayEpoch){displayStatusKnown=false;put('displayStatus',()=>t('โหลดการแสดง usage ไม่สำเร็จ'));languageReadback();}}
}
function pollDisplay(){setTimeout(async()=>{await refreshDisplay();pollDisplay();},15000);}
$('brightnessPercent').addEventListener('input',languageReadback);
for(const key of ['brightnessAuto','brightnessDayStart','brightnessNightStart','codexUsageMode'])$(key).addEventListener('change',languageReadback);
$('language').addEventListener('change',()=>{applyLanguage($('language').value);languageReadback();});
$('display').addEventListener('submit',async e=>{
 e.preventDefault();if(displaySaving)return;
 try{
  if(!displayLoaded)throw issue(()=>t("รอโหลดการแสดง usage"));
  const patch={};
  const selected=Object.fromEntries(displayKeys.map(key=>[key,editorValue(key)]));
  if(!validTime(selected.brightnessDayStart)||!validTime(selected.brightnessNightStart)||selected.brightnessDayStart===selected.brightnessNightStart)throw issue(()=>t('เวลาต้องเป็น HH:MM และไม่ซ้ำกัน'));
  if(!validDisplay(selected))throw issue(()=>t('โหลดการแสดง usage ไม่สำเร็จ'));
  for(const key of displayKeys)if(selected[key]!==displaySaved[key])patch[key]=selected[key];
  if(!Object.keys(patch).length){put('displayStatus',()=>t("ไม่มีการเปลี่ยนแปลง"));return;}
  ++displayEpoch;displaySaving=true;$('displayButton').disabled=true;put('displayStatus',()=>t("กำลังบันทึก..."));
  const r=await fetch('/api/display',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(patch)});
  if(!r.ok)throw issue(()=>r.status===503?t("ยังบันทึกไม่ได้ กรุณาลองอีกครั้ง"):t("ตรวจสอบค่า / สิทธิ์ (HTTP ")+r.status+')');
  // Submitted values are clean after acceptance, but edits made during POST
  // or its GET remain dirty against this private request-time baseline.
  displayReconcile=selected;displayLoaded=false;displaySaved=undefined;languageReadback();
  try{await loadDisplay();}catch(_){put('displayStatus',()=>t("บันทึกแล้ว แต่โหลดการแสดงผลไม่สำเร็จ กรุณาโหลดหน้าใหม่"));return;}
  put('displayStatus',()=>t("บันทึกการแสดง usage แล้ว"));
 }catch(err){put('displayStatus',()=>errorText(err));}
 finally{displaySaving=false;$('displayButton').disabled=!displayLoaded;}
});
loadDisplay().catch(_=>{put('displayStatus',()=>t("โหลดการแสดง usage ไม่สำเร็จ"));languageReadback();});pollDisplay();
function selectedFile(){put('firmwareSelected',()=>$('firmwareFile').files?.[0]?.name||t("ยังไม่เลือกไฟล์"));}
$('firmwareFile').addEventListener('change',selectedFile);selectedFile();
let firmwareInfo;
let updating=false;
async function firmwareStatus(){
 const controller=new AbortController();const timeout=setTimeout(()=>controller.abort(),2000);
 try{
  const r=await fetch('/api/firmware',{cache:'no-store',signal:controller.signal});
  if(!r.ok)throw issue(()=>t("Firmware status unavailable (HTTP ")+r.status+')');
  firmwareInfo=await r.json();
  put('firmwareInfo',()=>t("Version ")+firmwareInfo.version+' · '+firmwareInfo.capacity+t(" bytes capacity")+(firmwareInfo.busy?t(" · Update busy"):''));
  return firmwareInfo;
 }finally{clearTimeout(timeout);}
}
function updateFailed(render){updating=false;$('firmwareButton').disabled=false;put('firmwareStatus',render);}
async function reconnect(previous){
 put('firmwareStatus',()=>t("Validated. Restarting device; waiting to reconnect..."));
 for(let attempt=0;attempt<30;attempt++){
  await new Promise(resolve=>setTimeout(resolve,2000));
  try{
   const info=await firmwareStatus();
   if(!info.busy && info.runningPartition!==previous){
    updating=false;$('firmwareButton').disabled=false;put('firmwareStatus',()=>t("Update complete. Connected to version ")+info.version);return;
   }
  }catch(_){}
 }
 updateFailed(()=>t("Reconnect could not be confirmed. Check the device and reload this page to retry."));
}
$('firmware').addEventListener('submit',async e=>{
 e.preventDefault();if(updating)return;
 try{
  const file=$('firmwareFile').files[0];
  if(!file || !file.name.endsWith('.bin') || file.name==='desk-duo-cyd.bin')throw issue(()=>t("Choose an application .bin (desk-duo-app.bin)."));
  updating=true;$('firmwareButton').disabled=true;
  const info=await firmwareStatus();
  if(info.busy)throw issue(()=>t("Firmware maintenance busy. Retry shortly."));
  if(!file.size || file.size>info.capacity)throw issue(()=>t("Application size exceeds slot capacity or file is empty."));
  const req=new XMLHttpRequest();req.open('POST','/api/firmware');req.timeout=130000;
  req.setRequestHeader('Content-Type','application/octet-stream');req.setRequestHeader('X-DeskDuo-Update','1');
  $('firmwareProgress').value=0;put('firmwareStatus',()=>t("Uploading... keep the device powered."));
  req.upload.onprogress=e=>{if(e.lengthComputable){$('firmwareProgress').value=100*e.loaded/e.total;if(e.loaded===e.total)put('firmwareStatus',()=>t("Upload sent. Verifying application..."));}};
  req.onerror=req.ontimeout=req.onabort=()=>updateFailed(()=>t("Upload interrupted. Check device status and retry."));
  req.onload=()=>{if(req.status===200)return reconnect(info.runningPartition);updateFailed(()=>req.status===503?t("Firmware maintenance busy. Retry shortly."):t("Update rejected (HTTP ")+req.status+t("). Check the application file and retry."));};
  req.send(file);
 }catch(err){updateFailed(()=>errorText(err));}
});
function weatherStatusText(info){
 const w=info.weather,a=info.attempt;
 if(typeof info.stationOnline!=='boolean' || typeof info.weatherConfigured!=='boolean' || !w || typeof w.hasData!=='boolean' || !a || typeof a.attempted!=='boolean')throw Error('invalid status');
 let state;
 if(!info.weatherConfigured)state=t("ยังไม่ตั้งค่าแหล่งข้อมูล");
 else if(!info.stationOnline)state=t("เครือข่ายยังไม่เชื่อมต่อ");
 else if(a.attempted && !a.currentGeneration)state=t("รอดึงข้อมูลสำหรับการตั้งค่าปัจจุบัน");
 else if(!a.attempted || a.outcome==='running')state=t("รอดึงข้อมูล");
 else if(a.outcome==='clock_wait')state=t("รอเวลาของเครื่องก่อนเชื่อมต่อ HTTPS");
 else if(a.outcome==='accepted')state=t("รับข้อมูลแล้ว");
 else if(a.outcome==='parse_failure')state=t("รูปแบบข้อมูลไม่ถูกต้อง");
 else if(a.outcome==='http_status')state=t("แหล่งข้อมูลตอบ HTTP ")+(Number.isInteger(a.httpStatus)?a.httpStatus:t("ไม่ทราบ"));
 else if(a.outcome==='tls_failure')state=t("เชื่อมต่อ HTTPS ไม่สำเร็จ");
 else state=t("เครือข่าย / การดึงข้อมูลไม่สำเร็จ");
 const values=[];
 if(w.hasData){
  for(const [key,unit] of [['temperature','°C'],['humidity','%']])if(Number.isFinite(w[key]))values.push(w[key]+unit);
  if(Number.isInteger(w.weatherCode))values.push(t("รหัสอากาศ ")+w.weatherCode);
  if(Number.isInteger(w.hourlyCount))values.push(t("รายชั่วโมง ")+w.hourlyCount);
  if(Number.isFinite(w.ageMs))values.push(t("อายุ ")+Math.floor(w.ageMs/1000)+t(" วินาที"));
 }
 return t("ข้อมูลอากาศ: ")+(w.hasData?(w.stale?t("ข้อมูลเก่า"):t("มีข้อมูล"))+(values.length?' · '+values.join(' · '):''):t("ยังไม่มีข้อมูล"))+' · '+state;
}
let networkLoading=false,networkRevision=0;
function invalidateNetworkStatus(){
 networkRevision++;put('weatherDataStatus',()=>t("ข้อมูลอากาศ: รออ่านสถานะสำหรับการตั้งค่าปัจจุบัน"));
}
async function loadNetwork(){
 if(networkLoading || updating)return;
 networkLoading=true;const revision=networkRevision;$('networkRefresh').disabled=true;
 const controller=new AbortController();const timeout=setTimeout(()=>controller.abort(),2000);
 try{
  const r=await fetch('/api/network',{cache:'no-store',signal:controller.signal});if(!r.ok)throw Error('status failed');
  const info=await r.json();weatherStatusText(info);
  if(revision===networkRevision)put('weatherDataStatus',()=>weatherStatusText(info));
 }catch(_){if(revision===networkRevision)put('weatherDataStatus',()=>t("ข้อมูลอากาศ: ไม่ทราบ (อ่านสถานะไม่สำเร็จ กรุณาลองอีกครั้ง)"));}
 finally{clearTimeout(timeout);networkLoading=false;$('networkRefresh').disabled=false;}
}
$('networkRefresh').addEventListener('click',loadNetwork);
loadNetwork();
firmwareStatus().catch(e=>put('firmwareInfo',()=>errorText(e)));
</script></html>)HTML";
}
