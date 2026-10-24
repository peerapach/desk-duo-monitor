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
<style>
*{box-sizing:border-box}[hidden]{display:none!important}body{margin:auto;max-width:1240px;padding:0 28px;background:#0b1012;color:#f1f4f2;font:14px/1.5 system-ui,-apple-system,sans-serif}h1,h2,h3,p{margin:0}h1{font-size:23px;letter-spacing:-.7px}h2{font-size:25px;letter-spacing:-.7px}h3{font-size:15px;font-weight:650;margin-bottom:12px}p,small{color:#a9b5b3}small{font-size:12px;overflow-wrap:anywhere}.app-header{display:flex;align-items:center;justify-content:space-between;min-height:80px;border-bottom:1px solid #293237}.header-tools{display:flex;align-items:center;gap:18px}.locale{display:flex;gap:2px}.locale button{background:transparent;color:#9eaaa8;padding:8px 10px;min-width:44px;border-radius:0}.locale button[aria-pressed=true]{color:#e5fff5;border-bottom:2px solid #a1e3cd}.app-layout{display:grid;grid-template-columns:172px minmax(0,1fr);gap:28px}.sidebar{padding:22px 14px 0 0;border-right:1px solid #293237}.sidebar button{position:relative;display:flex;align-items:center;gap:12px;width:100%;text-align:left;margin:4px 0;padding:12px;background:transparent;color:#c2cdca;font-size:13px}.sidebar button[aria-current=page]{background:#192526;color:#b7f4dc}.sidebar button[aria-current=page]:before{content:'';position:absolute;left:-7px;top:8px;bottom:8px;width:3px;border-radius:4px;background:#a1e3cd}.nav-icon{width:22px;height:22px;display:flex}.nav-icon svg{width:22px;height:22px;fill:none;stroke:currentColor;stroke-width:1.7;stroke-linecap:round;stroke-linejoin:round}main{min-width:0;padding:24px 0 36px}.section-head{position:relative;margin-bottom:20px}.section-head p{margin-top:3px}.section-select{display:none}.section-body{display:grid;grid-template-columns:minmax(0,1.55fr) minmax(220px,1fr);gap:16px;align-items:start}.editor-column,.saved-column{min-width:0}.card,fieldset,.panel{min-width:0;margin:0 0 12px;padding:17px;border:1px solid #303b40;border-radius:10px;background:linear-gradient(135deg,#192125,#12191c)}fieldset{border:0;padding:0;background:none}legend{display:none}label{display:block;margin:10px 0;font-size:12px;color:#c3ceca}input,select,textarea,button{font:inherit;box-sizing:border-box;max-width:100%}input:not([type=checkbox]),select,textarea{display:block;width:100%;min-height:44px;margin-top:5px;padding:9px 11px;color:#eef4f0;background:#1d272b;border:1px solid #455257;border-radius:7px}textarea{height:110px;resize:vertical}button,.filepick{min-height:44px;border:1px solid transparent;border-radius:7px;background:#a1e3cd;color:#09251b;padding:10px 16px;cursor:pointer;font-weight:600}button:disabled{opacity:.5}.secondary{background:#1c272b;color:#e3eeea;border-color:#4a595e}button:focus-visible,input:focus-visible,select:focus-visible,summary:focus-visible{outline:2px solid #a1e3cd;outline-offset:3px}input[type=checkbox]{appearance:none;position:relative;width:36px;height:22px;min-width:36px;border-radius:20px;background:#4e5b60;margin:0 8px 0 0;vertical-align:middle}input[type=checkbox]:before{content:'';position:absolute;left:3px;top:3px;width:16px;height:16px;border-radius:50%;background:#e8f1ed}input[type=checkbox]:checked{background:#81d2b5}input[type=checkbox]:checked:before{left:17px}label:has(input[type=checkbox]){display:flex;align-items:center;min-height:44px}input[type=range]{accent-color:#89CDC1;padding:0;border:0;background:transparent}#brightnessValue{font-size:36px;font-weight:600;letter-spacing:-1px;color:#fff9e9;display:block}.schedule,.paired{display:grid;grid-template-columns:1fr 1fr;gap:12px;align-items:end}.schedule label,.paired label{min-width:0}.day-track{height:12px;border-radius:12px;margin:14px 0 6px;background:linear-gradient(90deg,#4c5a63 25%,#f5e7c6 25%,#f5e7c6 75%,#4c5a63 75%)}.timeline{display:flex;justify-content:space-between;font-size:10px;color:#a9b5b3}.segmented{display:flex;border:1px solid #455257;border-radius:8px;padding:2px;margin:12px 0}.segmented button{flex:1;background:transparent;color:#c8d5d0;font-size:12px;padding:8px 4px;white-space:nowrap}.segmented button[aria-pressed=true]{background:#a1e3cd;color:#09251b}.control-backed{position:absolute;width:1px;height:1px;overflow:hidden;opacity:0;pointer-events:none}.provider-identities{display:grid;grid-template-columns:1fr 1fr;gap:10px}.provider{min-width:0;border:1px solid #354248;background:#11191c;padding:12px;border-radius:8px}.provider img{mix-blend-mode:screen;width:48px;height:48px;object-fit:contain;float:left;margin:0 10px 8px 0;border-radius:8px}.provider h3{margin:0 0 8px}.usage-meters{clear:both}.meter-row{margin-top:8px;font-size:11px}.meter-caption{display:flex;justify-content:space-between;gap:8px}.meter-caption strong{color:#f5faf7}.meter{height:7px;background:#39464e;border-radius:5px;margin-top:4px;overflow:hidden}.meter i{display:block;height:100%;background:#a1e3cd;border-radius:5px}#claudeMeters .meter i{background:#FB8452}#claudeMeters .meter-row:nth-child(2) .meter i{background:#E89C78}#codexMeters .meter i,#modePreview .meter i{background:#82CEBD}#codexMeters .meter-row:nth-child(2) .meter i,#modePreview .meter-row:nth-child(2) .meter i{background:#8DCFC2}.meter-unknown{color:#94a29e;font-size:12px}.saved-strip{white-space:pre-line;font-size:13px;color:#dbeae2}.saved-column>.card:first-child{border-color:#42574e}.saved-column h3{color:#e1f5e9}.saved-label{display:block;color:#94a89f;font-size:11px;margin-bottom:9px;text-transform:uppercase;letter-spacing:1px}.draft-indicator{color:#96b3a2;font-size:11px;display:block;margin-top:6px}.draft-indicator[data-dirty=true]{color:#f3ae7e}.brightness-card label{display:grid;grid-template-columns:90px 1fr;align-items:center;gap:8px;margin:0}.brightness-card label>span{display:none}.brightness-card input{min-height:44px}.schedule-card{position:relative}.schedule-card>label{position:absolute;right:15px;top:4px}.schedule-card>label span{display:none}.actions{position:sticky;bottom:0;z-index:2;background:#0b1012;display:flex;gap:12px;padding:12px 0;border-top:1px solid #293237;margin-top:12px}.actions button{flex:1}details{margin:12px 0}summary{cursor:pointer;min-height:44px;padding:12px 0;color:#bfcfc7}code{display:block;padding:12px;border:1px solid #455257;background:#1d272b;border-radius:7px;overflow-wrap:anywhere}.weather-hero{position:relative;min-height:170px}.weather-hero img{mix-blend-mode:screen;float:right;width:110px;height:110px;object-fit:contain}.hero-value{display:block;font-size:42px;font-weight:600;line-height:1.3;letter-spacing:-1.5px;color:#fff9e9}.hero-name{font-size:17px;font-weight:600;display:block;margin-bottom:8px}.hero-meta{display:block;color:#b3c5ba;font-size:12px;margin-top:9px}.neutral-weather{float:right;width:96px;height:82px;margin-top:15px;fill:#b5c4cd;opacity:.9}.quote-chart{width:100%;height:75px;display:block;margin:10px 0;fill:none;stroke:#a1e3cd;stroke-width:2}.stock-editor{display:grid;grid-template-columns:76px minmax(0,1fr) 62px;gap:8px;align-items:center;border-bottom:1px solid #29363b;padding:4px 0}.stock-editor label{margin:0;min-width:0}.stock-editor label:last-child{white-space:nowrap;font-size:11px;flex-wrap:wrap}.stock-editor select{padding:8px;min-width:76px}.stock-editor input{padding:8px}.stock-editor label:nth-child(2)>span,.stock-editor label:nth-child(2){font-size:0}.stock-editor input{font-size:14px}.stock-editor label:last-child input{margin:0}.stock-editor label:last-child>span{position:absolute;opacity:0;pointer-events:none}.watch-head{display:grid;grid-template-columns:76px 1fr 62px;gap:8px;font-size:11px;color:#9dada8;background:#202c31;padding:7px 0;border-radius:4px}.connection{font-size:18px;font-weight:600;color:#eff8f3;overflow-wrap:anywhere}#connectionState{display:flex;align-items:center;gap:6px;margin:8px 0;width:fit-content;padding:4px 8px;background:#162b24;border-radius:6px}#ssidStored{display:block}.badge{font-size:12px;color:#a6e7c9}.filepick{display:block;text-align:center;border:1px dashed #718881;background:#172322;color:#d8ece3;padding:30px 10px;margin:12px 0}input[type=file]{position:absolute;opacity:0;width:1px!important;height:1px;min-height:0!important;padding:0!important}.filepick:focus-within{outline:2px solid #a1e3cd}#firmwareButton{width:100%}#firmwareReview{border:1px solid #a1e3cd;border-radius:7px;padding:12px;color:#caffdf;margin:12px 0}progress{width:100%;accent-color:#a1e3cd;margin:16px 0}#status,#displayStatus,#firmwareStatus{white-space:pre-wrap;overflow-wrap:anywhere}#firmwareInfo{font-size:18px;white-space:pre-line}#settings,#display,#firmware{margin:0}#weatherDataStatus{font-size:12px;margin:12px 0}.section-content p:empty{display:none}
@media(max-width:1000px){.section-body{grid-template-columns:1fr}.saved-column{display:grid;grid-template-columns:1fr 1fr;gap:12px}.saved-column>.card{margin:0}.app-layout{gap:22px}}
@media(max-width:640px){body{padding:0 16px}.app-header{min-height:72px}.header-tools>small{display:none}.app-layout{display:block}.sidebar{display:none}.section-select{display:block;margin:14px 0 0}.section-select>span{display:none}main{padding-top:20px}.section-head{margin-bottom:16px}h2{font-size:23px}.section-body{display:flex;flex-direction:column;gap:0}.editor-column{width:100%;order:2}.saved-column{display:contents}.saved-column>.card{width:100%;order:3}.saved-column>.weather-hero,.saved-column>.quote-preview{order:1}.saved-column>.card{margin-bottom:12px}.card{padding:14px}.provider{padding:10px}.provider img{float:none;width:42px;height:42px;margin:0 0 6px}.provider-identities{gap:8px}.provider h3{font-size:14px}.actions{gap:8px}.actions button{font-size:12px;padding:10px 8px}.stock-editor{grid-template-columns:76px minmax(0,1fr) 58px;gap:6px}.watch-head{grid-template-columns:76px 1fr 58px;gap:6px}.stock-editor label:last-child{font-size:10px}.locale button{padding:8px}.hero-value{font-size:40px}}
</style>
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
 if(document.body&&$('headerTH')){for(const x of ['th','en'])$('header'+x.toUpperCase()).setAttribute('aria-pressed',String(language===x));refreshPresentation();}
}
function issue(render){const error=new Error();error.render=render;return error;}
function errorText(error){return error.render?error.render():t("ยังบันทึกไม่ได้ กรุณาลองอีกครั้ง");}
function parseJson(text){try{return JSON.parse(text);}catch(_){throw issue(()=>t("ตรวจสอบค่า JSON object"));}}

Object.assign(copy,{
 'รายการหุ้น':{th:'รายการหุ้น',en:'Stock watchlist'},'แหล่งข้อมูลหุ้น':{th:'แหล่งข้อมูลหุ้น',en:'Stock provider'},
 'ช่วงกราฟ':{th:'ช่วงกราฟ',en:'Chart range'},'วันนี้':{th:'วันนี้',en:'Today'},'5 วัน':{th:'5 วัน',en:'5 days'},
 'หุ้นไทยใส่ชื่อย่อ เช่น PTT เลือก TH; US เช่น AAPL ว่างเพื่อลบรายการ':{th:'หุ้นไทยใส่ชื่อย่อ เช่น PTT เลือก TH; US เช่น AAPL ว่างเพื่อลบรายการ',en:'Thai: select TH and enter PTT. US: AAPL. Leave a row blank to remove it.'},
 'แสดง':{th:'แสดง',en:'Show'},'ชื่อย่อหุ้น':{th:'ชื่อย่อหุ้น',en:'Ticker'},'ค่าหุ้นที่บันทึก: ':{th:'ค่าหุ้นที่บันทึก: ',en:'Saved stocks: '},
 'กรุณาตรวจชื่อหุ้น หุ้นซ้ำ หรือรอบดึง Yahoo ขั้นต่ำ 300 วินาที':{th:'กรุณาตรวจชื่อหุ้น หุ้นซ้ำ หรือรอบดึง Yahoo ขั้นต่ำ 300 วินาที',en:'Check tickers, duplicates and Yahoo refresh (minimum 300 seconds).'},
 'Yahoo เป็นช่องทางไม่เป็นทางการ อาจล่าช้าหรือจำกัดคำขอ ขั้นต่ำ 300 วินาที':{th:'Yahoo เป็นช่องทางไม่เป็นทางการ อาจล่าช้าหรือจำกัดคำขอ ขั้นต่ำ 300 วินาที',en:'Yahoo is unofficial. Quotes may be delayed or requests limited. Minimum refresh: 300 seconds.'},
 'JSON endpoint (เดิม)':{th:'JSON endpoint (เดิม)',en:'JSON endpoint (legacy)'}
});
for(const name of ['weather','stocks']){
 const box=document.createElement('fieldset');
 box.innerHTML=`<legend><span data-i18n="${name==='weather'?'อากาศ':'รายการหุ้น'}"></span></legend>
 <small id="${name}Stored"><span data-i18n="URL ที่บันทึก: กำลังโหลด...">URL ที่บันทึก: กำลังโหลด...</span></small>
 <label><span data-i18n="แทนที่ URL (ว่าง = คงค่าเดิม)">แทนที่ URL (ว่าง = คงค่าเดิม)</span><input id="${name}Url" type="url" maxlength="1024" placeholder="https://..."></label>
 <label><input id="${name}Clear" type="checkbox"><span data-i18n=" ปิดแหล่งข้อมูล / ล้าง URL"> ปิดแหล่งข้อมูล / ล้าง URL</span></label>
 <small id="${name}HeadersStored"><span data-i18n="Headers ปัจจุบัน: กำลังโหลด...">ส่วนหัวปัจจุบัน: กำลังโหลด...</span></small>
 <label><span data-i18n="แทนที่ headers เป็น JSON (ว่าง = คงค่าเดิม, {} = ล้าง)">แทนที่ส่วนหัวเป็น JSON (ว่าง = คงค่าเดิม, {} = ล้าง)</span><textarea id="${name}Headers" spellcheck="false" placeholder='{"Authorization":"Bearer ..."}'></textarea></label>
 <label><span data-i18n="Refresh วินาที (30–86400)">ดึงข้อมูลทุกกี่วินาที (30–86400)</span><input id="${name}Refresh" type="number" min="30" max="86400"></label>`;
 if(name==='stocks'){
  const old=box.innerHTML;
  box.innerHTML='<legend data-i18n="รายการหุ้น">รายการหุ้น</legend><label><span data-i18n="แหล่งข้อมูลหุ้น"></span><select id="stocksProvider"><option value="yahoo">Yahoo Finance</option><option value="json" data-i18n="JSON endpoint (เดิม)"></option></select></label><p id="stocksSelectionStored" role="status"></p><small data-i18n="หุ้นไทยใส่ชื่อย่อ เช่น PTT เลือก TH; US เช่น AAPL ว่างเพื่อลบรายการ"></small>';
  for(let i=0;i<5;i++)box.innerHTML+=`<div class="stock-editor"><label aria-label="Market ${i+1}"><select id="stockMarket${i}" aria-label="Market ${i+1}"><option value="TH">TH</option><option value="US">US</option></select></label><label><span data-i18n="ชื่อย่อหุ้น"></span> ${i+1}<input id="stockSymbol${i}" maxlength="20" autocomplete="off" spellcheck="false"></label><label><input id="stockEnabled${i}" type="checkbox"><span data-i18n="แสดง"></span></label></div>`;
  box.innerHTML+='<label><span data-i18n="ช่วงกราฟ"></span><select id="stocksRange"><option value="1d" data-i18n="วันนี้"></option><option value="5d" data-i18n="5 วัน"></option></select></label><small data-i18n="Yahoo เป็นช่องทางไม่เป็นทางการ อาจล่าช้าหรือจำกัดคำขอ ขั้นต่ำ 300 วินาที"></small><details><summary>JSON endpoint</summary>'+old.replace(/<legend>.*?<\/legend>/,'')+'</details>';
  // The refresh belongs to both providers and stays visible outside legacy details.
  box.innerHTML=box.innerHTML.replace(/(<label><span data-i18n="Refresh วินาที \(30–86400\)">[\s\S]*?<\/label>)(<\/details>)/,'$2$1');
 }
 if(name==='weather')box.innerHTML+='<p id="weatherDataStatus" role="status"><span data-i18n="ข้อมูลอากาศ: กำลังโหลด...">ข้อมูลอากาศ: กำลังโหลด...</span></p><button id="networkRefresh" type="button"><span data-i18n="อัปเดตสถานะ">อัปเดตสถานะ</span></button><small><span data-i18n="อ่านสถานะล่าสุดของเครื่อง การอ่านไม่สั่งดึงข้อมูลใหม่">อ่านสถานะล่าสุดของเครื่อง การอ่านไม่สั่งดึงข้อมูลใหม่</span></small><label><span data-i18n="ชื่อสถานที่">ชื่อสถานที่</span><input id="weatherLabel" maxlength="64"></label><label><span data-i18n="Field mappings (JSON paths)">จับคู่ฟิลด์ (JSON paths)</span><textarea id="weatherFields" spellcheck="false"></textarea></label>';
 $('sources').append(box);
}
applyLanguage('th');
let saved;
let configBaseline={};
let saving=false;
function stockReadback(){
 if(!saved)return;
 put('stocksSelectionStored',()=>{
  const st=saved.stocks;let dirty=true;
  try{dirty=$('stocksProvider').value!==st.provider||$('stocksRange').value!==st.chartRange||Number($('stocksRefresh').value)!==st.refreshSeconds||JSON.stringify(stockEditors())!==JSON.stringify(st.watchlist);}catch(_){}
  return t('ค่าหุ้นที่บันทึก: ')+st.provider+' · '+st.chartRange+' · '+(st.watchlist.filter(x=>x.enabled).map(x=>x.symbol).join(', ')||t('ยังไม่ตั้งค่า'))+(dirty?t(' · ยังไม่ได้บันทึก'):'');
 });
}
for(const id of ['stocksProvider','stocksRange','stocksRefresh',...Array.from({length:5},(_,i)=>['stockMarket'+i,'stockSymbol'+i,'stockEnabled'+i]).flat()])for(const event of ['input','change'])$(id).addEventListener(event,stockReadback);
const presence=value=>value===true?t("ตั้งค่าแล้ว"):value===false?t("ยังไม่ตั้งค่า"):t("ไม่ทราบ");
function stockEditors(){
 const list=[];
 for(let i=0;i<5;i++){
  let symbol=$('stockSymbol'+i).value.trim().toUpperCase();if(!symbol)continue;
  if($('stockMarket'+i).value==='TH'&&!symbol.endsWith('.BK'))symbol+='.BK';
  if(!/^[A-Z0-9][A-Z0-9.-]{0,19}$/.test(symbol)||list.some(x=>x.symbol===symbol))throw issue(()=>t('กรุณาตรวจชื่อหุ้น หุ้นซ้ำ หรือรอบดึง Yahoo ขั้นต่ำ 300 วินาที'));
  list.push({symbol,enabled:$('stockEnabled'+i).checked});
 }
 return list;
}
function unknownSettings(){
 saved=undefined;$('settingsButton').disabled=true;
 if(document.body)updateSectionState();
 put('stocksSelectionStored',()=>t('ค่าปัจจุบัน: ไม่ทราบ (โหลดไม่สำเร็จ)'));
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
  const st=current.stocks;
  if(st.provider===undefined){st.provider='json';st.chartRange='1d';st.watchlist=[];}
  if(!['json','yahoo'].includes(st.provider)||!['1d','5d'].includes(st.chartRange)||!Array.isArray(st.watchlist)||st.watchlist.length>5||st.watchlist.some(x=>!x||typeof x.symbol!=='string'||typeof x.enabled!=='boolean'))throw issue(()=>t('โหลดการตั้งค่าไม่สำเร็จ'));
  $('stocksProvider').value=st.provider;$('stocksRange').value=st.chartRange;
  for(let i=0;i<5;i++){const x=st.watchlist[i];$('stockMarket'+i).value=x&&!x.symbol.endsWith('.BK')?'US':'TH';$('stockSymbol'+i).value=x?x.symbol.replace(/\.BK$/,''):'';$('stockEnabled'+i).checked=x?x.enabled:true;}
  put('stocksSelectionStored',()=>t('ค่าหุ้นที่บันทึก: ')+st.provider+' · '+st.chartRange+' · '+(st.watchlist.filter(x=>x.enabled).map(x=>x.symbol).join(', ')||t('ยังไม่ตั้งค่า')));
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
  saved=current;if(document.body && $('weatherProvider'))syncWeatherCoordinates();configBaseline=captureConfigEditors();stockReadback();if(document.body)updateSectionState();$('settingsButton').disabled=false;
 }catch(_){unknownSettings();throw issue(()=>t("โหลดการตั้งค่าไม่สำเร็จ"));}
}
const configGroups={display:['autoRotateSeconds','touch'],wifi:['ssid','clearSsid','wifiPassword','clearWifi'],usage:['usageToken'],weather:['weatherUrl','weatherClear','weatherHeaders','weatherRefresh','weatherLabel','weatherFields'],stocks:['stocksUrl','stocksClear','stocksHeaders','stocksRefresh','stocksProvider','stocksRange',...Array.from({length:5},(_,i)=>['stockMarket'+i,'stockSymbol'+i,'stockEnabled'+i]).flat()]};
let weatherFriendlyEdited=false;
function captureConfigEditors(){return {...Object.fromEntries(Object.values(configGroups).flat().map(id=>[id,{value:$(id).value,checked:$(id).checked}])),weatherFriendlyEdited};}
function restoreConfigEditors(drafts,scope){if(!scope)return;if(scope!=='weather')weatherFriendlyEdited=drafts.weatherFriendlyEdited;for(const [group,ids] of Object.entries(configGroups))if(group!==scope)for(const id of ids){$(id).value=drafts[id].value;$(id).checked=drafts[id].checked;}stockReadback();if(document.body)updateSectionState();}
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
  const scope=e.submitter?.dataset?.scope||(document.body?activeSection:undefined);
  if(!scope || scope==='display'){
  const rotate=Number($('autoRotateSeconds').value);if(rotate!==saved.autoRotateSeconds)patch.autoRotateSeconds=rotate;
  const touch=editedObject($('touch').value,saved.touch);if(Object.keys(touch).length)patch.touch=touch;
  }
  if(!scope || scope==='wifi'){
  if($('clearSsid').checked)patch.ssid='';else if($('ssid').value && $('ssid').value!==saved.currentSSID)patch.ssid=$('ssid').value;
  if($('clearWifi').checked)patch.wifiPassword='';else if($('wifiPassword').value)patch.wifiPassword=$('wifiPassword').value;
  }
  if(!scope || scope==='usage')if($('usageToken').value)patch.usageToken=$('usageToken').value;
  for(const n of ['weather','stocks']){
   if(scope && scope!==n)continue;
   const source={};const refresh=Number($(n+'Refresh').value);if(refresh!==saved[n].refreshSeconds)source.refreshSeconds=refresh;
   // Never round-trip GET's redacted URL or any hidden secret.
   if($(n+'Clear').checked)source.url='';else if($(n+'Url').value)source.url=$(n+'Url').value;
   if($(n+'Headers').value.trim())source.headers=parseJson($(n+'Headers').value);
   if(n==='stocks'){
    const provider=$('stocksProvider').value,chartRange=$('stocksRange').value,watchlist=stockEditors();
    if(provider==='yahoo'&&(!Number.isInteger(refresh)||refresh<300||refresh>86400))throw issue(()=>t('กรุณาตรวจชื่อหุ้น หุ้นซ้ำ หรือรอบดึง Yahoo ขั้นต่ำ 300 วินาที'));
    if(provider!==saved.stocks.provider)source.provider=provider;
    if(chartRange!==saved.stocks.chartRange)source.chartRange=chartRange;
    if(JSON.stringify(watchlist)!==JSON.stringify(saved.stocks.watchlist))source.watchlist=watchlist;
   }
   if(n==='weather'){
    if(document.body && $('weatherProvider').value==='open-meteo'&&!$('weatherClear').checked && (weatherFriendlyEdited || ['weatherProvider','weatherLatitude','weatherLongitude'].some(id=>$(id).value!==configBaseline[id]?.value)))validateWeatherCoordinates();
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
  const drafts=captureConfigEditors();
  try{await load();restoreConfigEditors(drafts,scope);}catch(_){put('status',()=>t("บันทึกแล้ว แต่โหลดค่าปัจจุบันไม่สำเร็จ กรุณาโหลดหน้าใหม่ก่อนแก้ไขอีกครั้ง"));return;}
  put('status',()=>t("บันทึกแล้ว Wi-Fi จะเชื่อมต่อใหม่เมื่อเปลี่ยนค่า"));
 }catch(err){put('status',()=>errorText(err));}
 finally{saving=false;$('settingsButton').disabled=!saved;if(document.body)updateSectionState();}
});load().catch(e=>put('status',()=>errorText(e)));
let displayLoaded=false,displaySaved,displaySaving=false;
let displayEpoch=0,displayRequest,displayStatusKnown=false;
const displayKeys=['codexUsageMode','language','brightnessPercent','brightnessAuto','brightnessDayStart','brightnessNightStart'];
const editorValue=key=>key==='brightnessAuto'?$(key).checked:key==='brightnessPercent'?Number($(key).value):$(key).value;
// Start with actual editor defaults so changes before the first valid GET also
// survive. This private baseline never establishes public saved authority.
let displayReconcile=Object.fromEntries(displayKeys.map(key=>[key,editorValue(key)]));
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
  if(baseline && editorValue(key)===baseline[key]){
   if(key==='brightnessAuto')$(key).checked=current[key];else $(key).value=current[key];
  }
 }
 displaySaved=current;displayReconcile=current;displayLoaded=true;displayStatusKnown=true;
 $('displayButton').disabled=displaySaving;if(document.body)updateSectionState();applyLanguage($('language').value);languageReadback();
}
function languageReadback(){
 if(document.body)updateSectionState();
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
  const scope=e.submitter?.dataset?.scope||(document.body?activeSection:undefined);
  const keys=scope==='usage'?['codexUsageMode']:scope==='display'?displayKeys.filter(key=>key!=='codexUsageMode'):displayKeys;
  // Validate the candidate this section will actually persist. Other editors
  // remain independent drafts, including invalid schedules in Display.
  const candidate={...displaySaved,...Object.fromEntries(keys.map(key=>[key,selected[key]]))};
  if(!validTime(candidate.brightnessDayStart)||!validTime(candidate.brightnessNightStart)||candidate.brightnessDayStart===candidate.brightnessNightStart)throw issue(()=>t('เวลาต้องเป็น HH:MM และไม่ซ้ำกัน'));
  if(!validDisplay(candidate))throw issue(()=>t('โหลดการแสดง usage ไม่สำเร็จ'));
  for(const key of keys)if(selected[key]!==displaySaved[key])patch[key]=selected[key];
  if(!Object.keys(patch).length){put('displayStatus',()=>t("ไม่มีการเปลี่ยนแปลง"));return;}
  ++displayEpoch;displaySaving=true;$('displayButton').disabled=true;put('displayStatus',()=>t("กำลังบันทึก..."));
  const r=await fetch('/api/display',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(patch)});
  if(!r.ok)throw issue(()=>r.status===503?t("ยังบันทึกไม่ได้ กรุณาลองอีกครั้ง"):t("ตรวจสอบค่า / สิทธิ์ (HTTP ")+r.status+')');
  // Submitted values are clean after acceptance, but edits made during POST
  // or its GET remain dirty against this private request-time baseline.
  displayReconcile={...displaySaved,...patch};displayLoaded=false;displaySaved=undefined;languageReadback();
  try{await loadDisplay();}catch(_){put('displayStatus',()=>t("บันทึกแล้ว แต่โหลดการแสดงผลไม่สำเร็จ กรุณาโหลดหน้าใหม่"));return;}
  put('displayStatus',()=>t("บันทึกการแสดง usage แล้ว"));
 }catch(err){put('displayStatus',()=>errorText(err));}
 finally{displaySaving=false;$('displayButton').disabled=!displayLoaded;if(document.body)updateSectionState();}
});
loadDisplay().catch(_=>{put('displayStatus',()=>t("โหลดการแสดง usage ไม่สำเร็จ"));languageReadback();});pollDisplay();
let reviewedFile;
function selectedFile(){reviewedFile=undefined;if($('firmwareReview')){$('firmwareReview').hidden=true;const label=$('firmwareButton').querySelector('[data-i18n]');label.dataset.i18n='Review update';label.textContent=t('Review update');}put('firmwareSelected',()=>{const file=$('firmwareFile').files?.[0];return file?file.name+' · '+file.size+' '+t('bytes'):t("ยังไม่เลือกไฟล์");});}
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
  if(document.body && reviewedFile!==file){reviewedFile=file;updating=false;$('firmwareButton').disabled=false;$('firmwareReview').hidden=false;const label=$('firmwareButton').querySelector('[data-i18n]');label.dataset.i18n='Upload firmware';label.textContent=t('Upload firmware');put('firmwareReview',()=>t('Review update')+': '+file.name+' · '+file.size+' '+t('bytes')+' · '+t('Press upload again to install.'));return;}
  const req=new XMLHttpRequest();req.open('POST','/api/firmware');req.timeout=130000;
  req.setRequestHeader('Content-Type','application/octet-stream');req.setRequestHeader('X-DeskDuo-Update','1');
  $('firmwareProgress').hidden=false;$('firmwareProgress').value=0;put('firmwareStatus',()=>t("Uploading... keep the device powered."));
  req.upload.onprogress=e=>{if(e.lengthComputable){$('firmwareProgress').value=100*e.loaded/e.total;if(e.loaded===e.total)put('firmwareStatus',()=>t("Upload sent. Verifying application..."));}};
  req.onerror=req.ontimeout=req.onabort=()=>updateFailed(()=>t("Upload interrupted. Check device status and retry."));
  req.onload=()=>{if(req.status===200)return reconnect(info.runningPartition);updateFailed(()=>req.status===503?t("Firmware maintenance busy. Retry shortly."):t("Update rejected (HTTP ")+req.status+t("). Check the application file and retry."));};
  req.send(file);
 }catch(err){updateFailed(()=>errorText(err));}
});
function renderWeatherArtwork(info){
 const img=$('weatherArtwork');if(!img)return;
 const w=info.weather;let tile;
 if(w?.hasData){const code=w.weatherCode;
  if((code===0||code===1)&&typeof w.isDay==='boolean')tile=w.isDay?0:2;
  else if(code===2&&typeof w.isDay==='boolean')tile=w.isDay?1:3;
  else if(code===3||code===45||code===48||code===2)tile=4;
  else if([51,53,55,56,57,61,63,65,66,67,71,73,75,77,80,81,82,85,86,95,96,97,99].includes(code))tile=5;
 }
 img.hidden=tile===undefined;if(tile!==undefined){img.dataset.tile=String(tile);img.src=portalImages['weather'+tile];}
}
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
 // Match the device's two missed hourly observation periods; receipt age stays separate.
 const observationAge=Number.isInteger(info.epochSeconds)&&info.epochSeconds>=1700000000&&Number.isInteger(w.updatedAt)&&w.updatedAt>0?Math.max(0,info.epochSeconds-w.updatedAt):null;
 const observationStale=observationAge!==null&&observationAge>=7200;
 const values=[];
 if(w.hasData){
  for(const [key,unit] of [['temperature','°C'],['humidity','%']])if(Number.isFinite(w[key]))values.push(w[key]+unit);
  if(Number.isInteger(w.weatherCode))values.push(t("รหัสอากาศ ")+w.weatherCode);
  if(Number.isInteger(w.hourlyCount))values.push(t("รายชั่วโมง ")+w.hourlyCount);
  if(typeof w.updatedAt==='string' && w.updatedAt)values.push(t('Observation ')+w.updatedAt);
  if(Number.isFinite(w.updatedAt)&&w.updatedAt>0){const offset=Number.isFinite(w.utcOffsetSeconds)?w.utcOffsetSeconds:0;values.push(t('Observation ')+new Date((w.updatedAt+offset)*1000).toISOString().replace('T',' ').replace('.000Z','')+' UTC'+(offset>=0?'+':'')+(offset/3600));}
  if(observationAge!==null)values.push(t('Observation age ')+observationAge+t(' วินาที'));
  if(Number.isFinite(w.ageMs))values.push(t('Receipt age ')+Math.floor(w.ageMs/1000)+t(" วินาที"));
 }
 return t("ข้อมูลอากาศ: ")+(w.hasData?(w.stale||observationStale?t("ข้อมูลเก่า"):t("มีข้อมูล"))+(values.length?' · '+values.join(' · '):''):t("ยังไม่มีข้อมูล"))+' · '+state;
}
let lastNetwork=null;
let networkLoading=false,networkRevision=0;
function invalidateNetworkStatus(){
 networkRevision++;lastNetwork=null;refreshPresentation();if($('weatherArtwork'))$('weatherArtwork').hidden=true;put('weatherDataStatus',()=>t("ข้อมูลอากาศ: รออ่านสถานะสำหรับการตั้งค่าปัจจุบัน"));
}
async function loadNetwork(){
 if(networkLoading || updating)return;
 networkLoading=true;const revision=networkRevision;$('networkRefresh').disabled=true;
 const controller=new AbortController();const timeout=setTimeout(()=>controller.abort(),2000);
 try{
  const r=await fetch('/api/network',{cache:'no-store',signal:controller.signal});if(!r.ok)throw Error('status failed');
  const info=await r.json();weatherStatusText(info);
  if(revision===networkRevision){lastNetwork=info;renderWeatherArtwork(info);refreshPresentation();put('weatherDataStatus',()=>weatherStatusText(info));}
 }catch(_){if(revision===networkRevision){lastNetwork=null;refreshPresentation();if($('weatherArtwork'))$('weatherArtwork').hidden=true;put('weatherDataStatus',()=>t("ข้อมูลอากาศ: ไม่ทราบ (อ่านสถานะไม่สำเร็จ กรุณาลองอีกครั้ง)"));}}
 finally{clearTimeout(timeout);networkLoading=false;$('networkRefresh').disabled=false;}
}
$('networkRefresh').addEventListener('click',loadNetwork);
loadNetwork();
firmwareStatus().catch(e=>put('firmwareInfo',()=>errorText(e)));
Object.assign(copy,{
 'Display':{th:'จอแสดงผล',en:'Display'},'AI usage':{th:'การใช้งาน AI',en:'AI usage'},'Weather':{th:'สภาพอากาศ',en:'Weather'},'Stocks':{th:'หุ้น',en:'Stocks'},'Wi-Fi':{th:'Wi-Fi',en:'Wi-Fi'},'Firmware':{th:'เฟิร์มแวร์',en:'Firmware'},
 'Section':{th:'ส่วนการตั้งค่า',en:'Section'},'Device settings':{th:'ตั้งค่าอุปกรณ์',en:'Device settings'},'Saved on device':{th:'ค่าที่บันทึกในเครื่อง',en:'Saved on device'},'Save changes':{th:'บันทึกการเปลี่ยนแปลง',en:'Save changes'},'Discard':{th:'ยกเลิกการแก้ไข',en:'Discard'},
 'Display preferences':{th:'ความสว่าง ตารางเวลา และภาษาของจอ',en:'Brightness, schedule and screen language'},'Usage preferences':{th:'ตั้งค่าช่วงการใช้งานและโทเค็นรับข้อมูล',en:'Usage windows and authenticated usage hook'},'Weather preferences':{th:'ตำแหน่งที่ตั้งและแหล่งพยากรณ์อากาศ',en:'Location and forecast source'},'Stock preferences':{th:'รายการหุ้น ตลาด และช่วงกราฟ',en:'Watchlist, markets and chart range'},'Wi-Fi preferences':{th:'ตั้งค่าเครือข่ายของอุปกรณ์',en:'Configure the device network'},'Firmware preferences':{th:'ตรวจสอบไฟล์แอปก่อนอัปเดตอุปกรณ์',en:'Review an application before updating the device'},
 'Advanced':{th:'ขั้นสูง',en:'Advanced'},'Weather provider':{th:'แหล่งข้อมูลอากาศ',en:'Weather provider'},'Custom endpoint':{th:'URL กำหนดเอง',en:'Custom endpoint'},'Latitude':{th:'ละติจูด',en:'Latitude'},'Longitude':{th:'ลองจิจูด',en:'Longitude'},'Coordinates require valid latitude and longitude.':{th:'กรุณาระบุละติจูดและลองจิจูดที่ถูกต้อง',en:'Coordinates require valid latitude and longitude.'},
 'Edit coordinates to generate an Open-Meteo endpoint. Custom URLs stay unchanged until you choose Open-Meteo.':{th:'แก้ไขพิกัดเพื่อสร้าง URL Open-Meteo ค่า URL เดิมจะคงอยู่จนกว่าจะเลือก Open-Meteo',en:'Edit coordinates to generate an Open-Meteo endpoint. Custom URLs stay unchanged until you choose Open-Meteo.'},
 'bytes':{th:'ไบต์',en:'bytes'},'Review update':{th:'ตรวจสอบการอัปเดต',en:'Review update'},'Press upload again to install.':{th:'กดอัปโหลดอีกครั้งเพื่อติดตั้ง เครื่องจะเริ่มใหม่',en:'Press upload again to install. The device will restart.'},'Receipt age ':{th:'รับข้อมูลมาแล้ว ',en:'Receipt age '},'Observation ':{th:'เวลาสังเกตการณ์ ',en:'Observation '},'Observation age ':{th:'อายุข้อมูลจากแหล่ง ',en:'Observation age '},'Usage hook':{th:'จุดรับข้อมูลการใช้งาน',en:'Usage hook'}
});
/* PORTAL_IMAGES_START */
const portalImages={"claude":"data:image/png;base64,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","codex":"data:image/png;base64,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","weather0":"data:image/png;base64,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","weather1":"data:image/png;base64,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","weather2":"data:image/png;base64,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","weather3":"data:image/png;base64,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","weather4":"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAFAAAABQCAMAAAC5zwKfAAADAFBMVEUAAADc29vf3t7d3NwDAwPa2dnR0NABAQECAgLX1tbZ2NjV1NTW1dWzsrPNzMzT0tLb2trS0dHKycnQz8/U09Nqamve3d3GxcXZ19i4uLiVlJXg39/JyMjMy8vCwcFubm9ra2yop6i7u7vY19eko6TAv790dHWtra26ubnPzs6qqqq8u7ypqanFxMRlZWZnZ2jBwMHa2dm3trZ8fH19fX6Lioq9vL2sq6u+vr6hoKGnpqeDg4THxsZoaGliYmPLystkZGWbmpu2tbaysbGzsrIrKyy+vb28vLx1dXZhYWK7urqIiIiYl5ign6C5uLmBgYJtbW5+fn+FhIWcnJwuLy/Ew8OMjIx7e3zX1tbOzc5paWm/vr56enqampqhoaGCgoNgYGF7ent2dnevrq9vb3C0s7NycnOfnp+Qj5Cura0pKivOzc1lZmZjY2TIx8jf399mZmfd3NyUk5SOjY7GxsaDg4OmpaaAf4BxcXKxsLC4t7isq6zR0NCRkZGjoqOXlpfR0dHEwsLe3d2TkpOPjo+Hh4iSkZKAgIFwcHCwr69pamuTk5Nzc3TQzs+Mi4uioqPW1NWwr7AFBQWnp6i1s7TKycnT09PAwMClpKSJiYrZ19ibm5uZmJl5eHmPj5B0dHS3t7fW1dV2dnadnJ2ura4KCgrh4ODb2tlubm7BwcFkZWZjY2XLysrb29uGhoYxMTLCwsLS0tLU1NS9vb2VlZUeHh5tbG0SEhK7ubqtrKzk4+Pn5eWGhYYoKSnPz8+1tLSEhIZmZ2jDw8PU0tOysbGenZ53d3iJiYlCQkNvb28zNDSWlpYPDg7Y2Njc2totLS4oJymLi4zPzs5naGnW1tZ4eHhxcnK1tLXIxsdbWls7Ojrc2tssLC3b2dqenZ0vMDBkZmgiIiJGR0dPTk/i4eEVFRUgICHp6OiCg4QmJic1NTYZGRmKi4wICAhWWFlQUFK5ubo5OTk+Pz9ISUkbGhpBQEG2trWurq8MDA2+v74jJCS1tbVMS0tXVlbMyspeXV3++YOAAAAACXBIWXMAAAPoAAAD6AG1e1JrAAAI5UlEQVRYw+2XeVRTVx7HX5ZnQhKyIycsARICHMLiQdEAQYGkgAgCQTCKoghBNGJzgiCEgDYICi4I9RAFFI5bXUChihZX1AMuuIFacatat+7tdJ9htt990Y62nRmlc+Yvvrx7E97yud/f7973y3sYNqIRjWhEI3pN0WjU56L9L2hU6iv/0f8oDkzVdd5+NHFmwK2/XjgMu+h/BAm4uj+N0wnuOT9+TKZwte/Ez+0E5HAjp9GxHbfUlqPkhvVMDk+wdsAstMOVmqvowHAEsZ0THX2MSxkNUi0LkDzzAHvSgJTffBEbDhHsFRw5gnNJJC4ORACazYIBPZs/ibfx/jCIdOxJ6DMcp1goAGRIWes5vFGIN4m/ONUl5uM3JtKwJ8VHtFwKhUIigFqWUMgzr2W7troMKsW7rY/ekEjDdhQeYXHb2kgkAsggsrhcoHfluyj9T1sVus/ejEjHlj5m4vg97mUShMy9hzMa7FjMjuUnZri6LEkVyySSmu8x2pvwfmAIWVIGENvuECa5XDDJFI46weYvOW1NjzEpCuhvFLCKwWGy7Bg4t40EebRYSMilHbPDzG5VpsoUMRLRNz+8ftBUbC5FUC9kaqV4G9BGk0eTLaQ7XPySVsgbcF3sP0Yh+cYkCq573aDBYJUdj8NhahtwkoVMJr8NzUK5gzO0zHoB28Vxt0IiilKVXHgtizQoVtg5s57H4wlZjDaKxdl5p7Oz89tAhDSyhKP0fEexQmLaUJUV/l8d0mwVj45ptK0z1poBiAw679zpvBMBSTiurTfr+cp2hU+Uuiqr9GdUPTA69d9UCxpR+OouXqTvyOpgs/UCodYGRERnyCKsR5bQzAZgtM+QWrU94+kvFn+npqHKd/G75AS1LF11ksNZbjZ3MKW2FELEziiJbbj0OgIuOZ3u86NO1VUY8A+Nsa/yWMTtw7+taWDv+xvtR59B2aOQyTguZcIkM3ASZbTFQka80RAxTApvLUxKu0xuGrIvvrnEPKNV6ejfLvc43/krIlS4v/39GblBCrcYi7leq5VqQQ043CgUMGmxQAbfY2hZZ8xwq/iLZTE+opY46xjFeLmPKUinEx3wvVX38pxDYd53lCIFY9rrLCYL6brWTtrAQNWLgkS6jEoOk2PWty4GYHRMnEQhS1cAL2qDvb3K+ydT4uGXJ5264qiWwbBDNYXDEQrfZzIBCz4vMXD88r17cJfAwfVCDpRYvhIBo+Xy8WBPFHRSXZz10/Yu33drz/6LSMeajwi165lnUBE1m2EJAvV95FRrd6mhgWEr2YATPOelH5DL5RIIe0inqt3uPcHX493QGt95db/wbmt56HSBYABJIDCP4nVw6uHmY7KusyCrLOYZKK9QX135g46pY2SQO3kcRHtSp6raXlLiV+hZVhOSUvPdizTS518agMHZrrBBY88YOLFWQGA5nHr4A+PE4Va+y6Bj+26ZIjpGEicKGtKpwV+Jr19oWcL8nm2Rkd22oOnYt+ZWdDIIOj6f39qKsGy9fkAAvyMnBIIZBMxlEOy1W/+sOEDwNuhUxVnevr4ehWUJ2yJnx/cZl9l+GehYYocjLCf/VH9HJKVSOTi4mABPctXrXYnBlOiI/2mxFdzJJR9MhXBhNsCeR2hh2fyUJsCVl5ZnU221pWRQLLZaZTKrWCzeDa39dKoNrgTTg45orHfaIVSYjJg4H5HJZGq5aa+q9Ua4jTUhPY3xB42lW1bk93/yOeDo2MOp4vSYuDiJRD4++sD48dGgdNkYq9W6uz0VQkRDpaeno7z5TDVB5uztIXlZXb5+HoWeGQk9KU19xsotacHBwVO6rwKOju0/AOvpx6iolhbTVJHIJBJN9fGJiyEEaw3GkX8AJFAUwFTFVVVZC7rAHNAyEj5qbIp3qszN/2JKRcW+sO79BPC2xF5dXKxSqdT29mo1fOhuDgUFtQC5JSgqaGjo5smTOjigBhagJoAzSJxnTcL8bY2RTcuM5Vvy0zZVdGsKEpNXXrA5rF2woKurBOTd1eXt7b1ge1ZtbbGKQBTX1gKFEKAQq3CjJ1p12xpnN8Uvg9xB6jZVaMISVy5NHpc8jQA+DfHzg6VeGAqCwaHz8/NFmjABGB7PFRq6EaEyQmDJpTQ29TkZjUaIFWgV3WErly4dN3bs9FkPYVJo2M+5HmVwYgjaMkISEjIyygh5biwEOxmwMyMBdsPi7UlJSYEw+1ZXoknNT+vfNOWYJmzOyuRxY6dPn57Zm0SsbBp2qCxlW0pk5OzZsyNtfWNjY2Rj5Ec983vgeggNadmyvoNG42qjE0S57uwU0D5N2CdzINCxmbM235jX2xvr9fXzW/mCU3lfPIrByclYXllZXm40lq92cjp40InYUZqbm7tlBeSqvz9tXT+wjmkK5hA5A2OZm2/0zov1WuPltSY7+8rzekMNz80tLc2FIPLTvggO3hSMlJaWti44LT8t+CwgKvYd69aEgQoSlyYDCEhgat7WWECtycnJKSoqysl2/+rLF+X14Zyz/euCz8J1kJFEpDkFYQWwAYHwkmxzkwnB9W7dOm/r1livnKJV2dnZbm5JSUnQubmdd7/2lxcFkYZ9qwnTFBDXQkbQZbMyIcvTx4JQgnqRl+bmNeCmaNWq7FXAcktyBzk4OFRXO1Sjr9Xut18q2bSPgYOmKnMzkZFmL1AsarHNACladf78rqRD4OUQQUHdzHBQYGDgKUN4+EyHmYHu5155/8AuHIoFTKxXc86HH0Igbtnn3dx2wbZrl43h4DATBATDIkOAAT4C4DMiLyJgT8Qiw6JT1Xs+o77yIEHDPn/kllOU7QZG3KthzPDwa4GEDIv2BIAikPLyJoImQ8sj+slz58LXCMO1U193wkPMr56QqJ3nbhnC3ZOSwE91eKDBEBCwZ8/evLy9eRPz9kbshcvhetSgBwo6ajA8eHDtweSvzj2l/vY5Bw1Qd7jz6rT7oGlXrty9e/ct0MIX3UKbjl+9evz4woVvXZl2H87cv3///U+ffEkY+r0Hr+G/Xv6Hh7nhaOSNfUQjGtGI/g/6JwOKUmRC3kKPAAAAAElFTkSuQmCC","weather5":"data:image/png;base64,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"};
/* PORTAL_IMAGES_END */
let activeSection='display';
function setupSections(){
 const names={display:'Display',usage:'AI usage',weather:'Weather',stocks:'Stocks',wifi:'Wi-Fi',firmware:'Firmware'},icons={display:'<rect x="3" y="4" width="18" height="13" rx="1"/><path d="M8 21h8m-4-4v4"/>',usage:'<path d="M9 15l6-6m-5-3L8 4a4 4 0 0 0-6 6l3 3m9 5 2 2a4 4 0 0 0 6-6l-3-3M6 10l4-4m4 12 4-4"/>',weather:'<path d="M6 18a4 4 0 0 1-1-8 7 7 0 0 1 13 0 4 4 0 0 1 0 8Z"/>',stocks:'<path d="M3 21V12h4v9m3 0V7h4v14m3 0V2h4v19"/>',wifi:'<path d="M2 8a16 16 0 0 1 20 0M5 12a11 11 0 0 1 14 0m-11 4a6 6 0 0 1 8 0m-4 4h.01"/>',firmware:'<rect x="5" y="5" width="14" height="14" rx="2"/><path d="M9 2v3m6-3v3M9 19v3m6-3v3M2 9h3m-3 6h3m14-6h3m-3 6h3"/>'};
 const header=document.createElement('header');header.className='app-header';header.innerHTML='<h1>Desk Duo</h1><div class="header-tools"><small data-i18n="Device settings"></small><div class="locale" aria-label="Portal language"><button id="headerTH" type="button">TH</button><button id="headerEN" type="button">EN</button></div></div>';document.body.prepend(header);
 document.querySelector('body>h1').remove();document.querySelector('body>p').remove();
 const layout=document.createElement('div');layout.className='app-layout';layout.innerHTML='<nav class="sidebar" aria-label="Desk Duo"></nav><div><label class="section-select"><span data-i18n="Section"></span><select id="sectionSelect"></select></label><main id="main"></main></div>';header.after(layout);
 for(const [name,title] of Object.entries(names)){
  const button=document.createElement('button');button.type='button';button.dataset.section=name;button.innerHTML='<span class="nav-icon" aria-hidden="true">'+'<svg viewBox="0 0 24 24">'+icons[name]+'</svg></span><span data-i18n="'+title+'"></span>';button.onclick=()=>selectSection(name);layout.querySelector('nav').append(button);
  const option=document.createElement('option');option.value=name;option.dataset.i18n=title;$('sectionSelect').append(option);
  const section=document.createElement('section');section.id='section-'+name;section.className='section-content';section.innerHTML='<div class="section-head"><h2 data-i18n="'+title+'"></h2><p data-i18n="'+({display:'Display',usage:'Usage',weather:'Weather',stocks:'Stock',wifi:'Wi-Fi',firmware:'Firmware'}[name])+' preferences"></p></div>';$('main').append(section);
 }
 // Preserve original form ownership while moving the one real editor for each field.
 for(const form of [$('settings'),$('display'),$('firmware')])for(const control of form.querySelectorAll('input,textarea,select,button'))control.setAttribute('form',form.id);
 const displayBox=$('display').querySelector('fieldset');$('section-display').append(displayBox);displayBox.querySelector('legend').dataset.i18n='Display';
 const usage=document.createElement('fieldset');usage.innerHTML='<legend data-i18n="AI usage"></legend><div class="provider-identities"><div class="provider"><img id="claudeIdentity" width="64" height="64" alt="" aria-hidden="true"><h3>Claude</h3></div><div class="provider"><img id="codexIdentity" width="64" height="64" alt="" aria-hidden="true"><h3>Codex</h3></div></div><small data-i18n="Usage preferences"></small><p data-i18n="Usage hook"></p><code>POST /api/usage</code>';$('section-usage').append(usage);
 $('claudeIdentity').src=portalImages.claude;$('codexIdentity').src=portalImages.codex;
 usage.append($('codexUsageMode').closest('label'),$('displayCurrent'),$('usageTokenStored'),$('usageToken').closest('label'));
 const schedule=document.createElement('div');schedule.className='schedule';for(const id of ['brightnessDayStart','brightnessNightStart'])schedule.append($(id).closest('label'));$('brightnessAuto').closest('label').after(schedule);
 const wifiBox=$('settings').querySelector('fieldset');$('section-wifi').append(wifiBox);wifiBox.querySelector('legend').dataset.i18n='Wi-Fi';
 displayBox.append($('autoRotateSeconds').closest('label'));const touch=$('touch').closest('fieldset');const advanced=document.createElement('details');advanced.innerHTML='<summary data-i18n="Advanced"></summary>';advanced.append(touch);$('section-display').append(advanced);
 $('section-weather').append($('weatherUrl').closest('fieldset'));$('section-stocks').append($('stocksProvider').closest('fieldset'));$('stocksRefresh').min='300';
 $('section-firmware').append($('firmware').querySelector('fieldset'));const review=document.createElement('p');review.id='firmwareReview';review.hidden=true;$('firmwareSelected').after(review);const reviewLabel=$('firmwareButton').querySelector('[data-i18n]');reviewLabel.dataset.i18n='Review update';
 $('section-display').append($('displayButton'),$('displayStatus'));$('section-wifi').append($('settingsButton'),$('status'));$('displayButton').dataset.scope='display';$('settingsButton').dataset.scope='wifi';
 const strip=document.createElement('div');strip.id='sectionState';strip.className='saved-strip';strip.setAttribute('role','status');$('main').append(strip);
 const actions=document.createElement('div');actions.className='actions';actions.innerHTML='<button id="sectionDiscard" type="button" class="secondary" data-i18n="Discard"></button><button id="sectionSave" type="button" data-i18n="Save changes"></button>';$('main').append(actions);
 $('displayButton').hidden=true;$('settingsButton').hidden=true;
 $('sectionSelect').onchange=()=>selectSection($('sectionSelect').value);
 $('sectionSave').onclick=()=>{
  if(activeSection==='firmware')return;
  const submit=(form)=>{const button=document.createElement('button');button.type='submit';button.formNoValidate=true;button.dataset.scope=activeSection;button.hidden=true;$(form).append(button);$(form).requestSubmit(button);button.remove();};
  if(activeSection==='display'||activeSection==='usage')submit('display');
  if(activeSection!=='display'||Number($('autoRotateSeconds').value)!==saved?.autoRotateSeconds||$('touch').value!==JSON.stringify(saved?.touch,null,2))submit('settings');
 };
 $('sectionDiscard').onclick=()=>{discardSection();updateSectionState();};
 document.addEventListener('input',updateSectionState);document.addEventListener('change',updateSectionState);
 setupWeatherEditor();const artwork=document.createElement('img');artwork.id='weatherArtwork';artwork.width=80;artwork.height=80;artwork.alt='';artwork.hidden=true;artwork.setAttribute('aria-hidden','true');$('weatherDataStatus').before(artwork);configGroups.weather.push('weatherProvider','weatherLatitude','weatherLongitude');buildPresentation();selectSection('display');applyLanguage(language);
}
function selectSection(name){
 activeSection=name;for(const node of document.querySelectorAll('.section-content'))node.hidden=node.id!=='section-'+name;
 for(const node of document.querySelectorAll('[data-section]')){if(node.dataset.section===name)node.setAttribute('aria-current','page');else node.removeAttribute('aria-current');}
 $('sectionSelect').value=name;$('sectionSave').parentElement.hidden=name==='firmware';$('sectionState').hidden=name==='firmware';
 if(name==='display'||name==='usage')$('section-'+name).append($('displayStatus'));
 if(name!=='firmware')$('section-'+name).querySelector('.editor-column').append($('status'));$('saved-'+name).append($('sectionState'));updateSectionState();if(name==='usage'||name==='stocks')loadDashboard();if(name==='weather'||name==='wifi')loadNetwork();
}
function updateSectionState(){
 if(!$('sectionState'))return;refreshPresentation();
 $('sectionSave').disabled=(activeSection==='display'||activeSection==='usage')?(!displayLoaded||displaySaving):(!saved||saving);
 put('sectionState',()=>{
  let text=t('Saved on device')+': ',dirty=false;
  if(activeSection==='display'||activeSection==='usage'){const keys=displayKeys.filter(key=>activeSection==='usage'?key==='codexUsageMode':key!=='codexUsageMode');dirty=!!displaySaved&&keys.some(key=>editorValue(key)!==displaySaved[key]);text+=displaySaved?(activeSection==='usage'?t(displaySaved.codexUsageMode==='both'?'5h + Weekly':'Weekly อย่างเดียว'):displaySaved.brightnessPercent+'% · '+displaySaved.brightnessDayStart+'–'+displaySaved.brightnessNightStart+' · '+displaySaved.language):t('ไม่ทราบ');}
  if(saved){const group=configGroups[activeSection]||[];dirty=dirty||group.some(id=>configBaseline[id]&&($(id).value!==configBaseline[id].value||$(id).checked!==configBaseline[id].checked));if(activeSection==='wifi')text+=saved.currentSSID||t('ยังไม่ตั้งค่า');if(activeSection==='usage')text+=' · '+t('Usage token ปัจจุบัน: ')+presence(saved.usageTokenConfigured);if(activeSection==='weather')text+=saved.weather.label+' · '+saved.weather.refreshSeconds+'s';if(activeSection==='stocks')text+=saved.stocks.provider+' · '+saved.stocks.chartRange+' · '+saved.stocks.refreshSeconds+'s';}
  else if(activeSection!=='display')text+=t('ไม่ทราบ');
  if($('draft-'+activeSection)){$('draft-'+activeSection).textContent=dirty?t('Unsaved changes'):t('Saved on device');$('draft-'+activeSection).dataset.dirty=String(dirty);}
  return text+(dirty?t(' · ยังไม่ได้บันทึก'):'');
 });
}
function discardSection(){
 if(activeSection==='display'||activeSection==='usage'){
  if(displaySaved)for(const key of displayKeys.filter(key=>activeSection==='usage'?key==='codexUsageMode':key!=='codexUsageMode')){if(key==='brightnessAuto')$(key).checked=displaySaved[key];else $(key).value=displaySaved[key];}
  applyLanguage($('language').value);languageReadback();
 }
 if(!saved)return;
 const section=activeSection;
 if(section==='display'){$('autoRotateSeconds').value=saved.autoRotateSeconds;$('touch').value=JSON.stringify(saved.touch,null,2);}
 if(section==='wifi'){$('ssid').value=saved.currentSSID;$('wifiPassword').value='';$('clearSsid').checked=false;$('clearWifi').checked=false;}
 if(section==='usage')$('usageToken').value='';
 if(section==='weather'||section==='stocks'){
  const n=section;$(n+'Url').value='';$(n+'Headers').value='';$(n+'Clear').checked=false;$(n+'Refresh').value=saved[n].refreshSeconds;
  if(n==='weather'){$('weatherLabel').value=saved.weather.label;$('weatherFields').value=JSON.stringify(saved.weather.fields,null,2);syncWeatherCoordinates();}
  else{$('stocksProvider').value=saved.stocks.provider;$('stocksRange').value=saved.stocks.chartRange;for(let i=0;i<5;i++){const row=saved.stocks.watchlist[i];$('stockMarket'+i).value=row&&!row.symbol.endsWith('.BK')?'US':'TH';$('stockSymbol'+i).value=row?row.symbol.replace(/\.BK$/,''):'';$('stockEnabled'+i).checked=row?row.enabled:true;}stockReadback();}
 }
}
function recognizedWeatherUrl(value){try{const url=new URL(value);return url.protocol==='https:'&&url.hostname==='api.open-meteo.com'&&!url.username&&!url.password&&!url.port&&!url.hash&&url.pathname==='/v1/forecast'?url:null;}catch(_){return null;}}
function syncWeatherCoordinates(){
 const url=recognizedWeatherUrl($('weatherUrl').value||saved?.weather.url);$('weatherProvider').value=url?'open-meteo':'custom';
 const metadata=!$('weatherUrl').value&&url&&Number.isFinite(saved?.weather.latitude)&&Math.abs(saved.weather.latitude)<=90&&Number.isFinite(saved?.weather.longitude)&&Math.abs(saved.weather.longitude)<=180?saved.weather:null;
 $('weatherLatitude').value=metadata?.latitude??url?.searchParams.get('latitude')??'';$('weatherLongitude').value=metadata?.longitude??url?.searchParams.get('longitude')??'';
 weatherFriendlyEdited=false;
}
function validateWeatherCoordinates(){
 const lat=$('weatherLatitude').value,lon=$('weatherLongitude').value;
 if(!lat||!lon||!Number.isFinite(Number(lat))||Math.abs(Number(lat))>90||!Number.isFinite(Number(lon))||Math.abs(Number(lon))>180)throw issue(()=>t('Coordinates require valid latitude and longitude.'));
 return {lat,lon};
}
function deriveWeatherUrl(){
 if($('weatherProvider').value!=='open-meteo')return;
 const {lat,lon}=validateWeatherCoordinates();
 const url=new URL('https://api.open-meteo.com/v1/forecast');url.searchParams.set('latitude',lat);url.searchParams.set('longitude',lon);url.searchParams.set('current','temperature_2m,relative_humidity_2m,weather_code,is_day');url.searchParams.set('hourly','temperature_2m,weather_code,is_day,precipitation_probability');url.searchParams.set('timezone','auto');$('weatherUrl').value=url.href;$('weatherClear').checked=false;
 const fields=parseJson($('weatherFields').value);Object.assign(fields,{temperature:'current.temperature_2m',humidity:'current.relative_humidity_2m',weatherCode:'current.weather_code',updatedAt:'current.time',utcOffsetSeconds:'utc_offset_seconds',isDay:'current.is_day',hourlyTimes:'hourly.time',hourlyTemperatures:'hourly.temperature_2m',hourlyRainProbability:'hourly.precipitation_probability',hourlyWeatherCodes:'hourly.weather_code',hourlyIsDay:'hourly.is_day'});$('weatherFields').value=JSON.stringify(fields,null,2);
}
function setupWeatherEditor(){
 const box=$('weatherUrl').closest('fieldset'),friendly=document.createElement('div');friendly.innerHTML='<label><span data-i18n="Weather provider"></span><select id="weatherProvider"><option value="custom" data-i18n="Custom endpoint"></option><option value="open-meteo">Open-Meteo</option></select></label><label><span data-i18n="Latitude"></span><input id="weatherLatitude" type="number" min="-90" max="90" step="any"></label><label><span data-i18n="Longitude"></span><input id="weatherLongitude" type="number" min="-180" max="180" step="any"></label><small data-i18n="Edit coordinates to generate an Open-Meteo endpoint. Custom URLs stay unchanged until you choose Open-Meteo."></small>';box.prepend(friendly);friendly.prepend($('weatherLabel').closest('label'));
 const advanced=document.createElement('details');advanced.innerHTML='<summary data-i18n="Advanced"></summary>';box.append(advanced);
 for(const id of ['weatherStored','weatherUrl','weatherClear','weatherHeadersStored','weatherHeaders','weatherFields']){const node=$(id);advanced.append(node.closest('label')||node);}
 for(const id of ['weatherLatitude','weatherLongitude'])$(id).addEventListener('input',()=>{weatherFriendlyEdited=true;});
 for(const id of ['weatherProvider','weatherLatitude','weatherLongitude'])$(id).addEventListener('change',()=>{weatherFriendlyEdited=true;try{deriveWeatherUrl();}catch(error){put('status',()=>errorText(error));}});
 syncWeatherCoordinates();
}

Object.assign(copy,{
 'Unsaved changes':{th:'ยังไม่ได้บันทึก',en:'Unsaved changes'},'Base brightness':{th:'ความสว่างฐาน',en:'Base brightness'},'Scheduled brightness':{th:'ตารางความสว่าง',en:'Scheduled brightness'},'Language':{th:'ภาษาของจอ',en:'Device language'},'Service selection':{th:'บริการที่ติดตาม',en:'Service selection'},'Codex window':{th:'ช่วงการใช้งาน Codex',en:'Codex window'},'Data source':{th:'แหล่งข้อมูล',en:'Data source'},'Location':{th:'ตำแหน่งที่ตั้ง',en:'Location'},'Data status':{th:'สถานะข้อมูล',en:'Data status'},'Weather preview':{th:'อากาศที่บันทึก',en:'Saved location weather'},'Quote preview':{th:'ราคาหุ้นล่าสุด',en:'Quote preview'},'Device preview':{th:'การแสดงผลที่บันทึก',en:'Saved display'},'Current connection':{th:'การเชื่อมต่อปัจจุบัน',en:'Current connection'},'Edit Wi-Fi network':{th:'แก้ไขเครือข่าย Wi-Fi',en:'Edit Wi-Fi network'},'Device information':{th:'ข้อมูลอุปกรณ์',en:'Device information'},'Upload firmware':{th:'อัปโหลดเฟิร์มแวร์',en:'Upload firmware'},'No live data':{th:'ยังไม่มีข้อมูลจริง',en:'No live data'},'Unknown day / night':{th:'ยังไม่ทราบกลางวัน / กลางคืน',en:'Unknown day / night'},'Clear':{th:'ท้องฟ้าแจ่มใส',en:'Clear'},'Partly cloudy':{th:'มีเมฆบางส่วน',en:'Partly cloudy'},'Cloudy':{th:'เมฆมาก',en:'Cloudy'},'Rain / precipitation':{th:'ฝน / หยาดน้ำฟ้า',en:'Rain / precipitation'},'Humidity':{th:'ความชื้น',en:'Humidity'},'Connected':{th:'เชื่อมต่อแล้ว',en:'Connected'},'Offline':{th:'ออฟไลน์',en:'Offline'},'Market':{th:'ตลาด',en:'Market'},'Ticker':{th:'ชื่อย่อ',en:'Ticker'},'Last 5 hours':{th:'5 ชั่วโมงล่าสุด',en:'Last 5 hours'},'Weekly total':{th:'รวมรายสัปดาห์',en:'Weekly total'},'Portal language only':{th:'เปลี่ยนภาษาเว็บเท่านั้น ภาษาจอบันทึกในหน้าจอแสดงผล',en:'Portal language only. Save device language in Display.'},'Actual received data':{th:'ข้อมูลที่เครื่องรับจริง',en:'Actual received data'},'No price history':{th:'ยังไม่มีประวัติราคา',en:'No price history'},'Stale':{th:'ข้อมูลเก่า',en:'Stale'},'Show':{th:'แสดง',en:'Show'},'Row':{th:'แถว',en:'row'},'Remove file':{th:'นำไฟล์ออก',en:'Remove file'}
});
let dashboard=null,dashboardLoading=false;
function card(title,parent){const node=document.createElement('div');node.className='card';const h=document.createElement('h3');h.dataset.i18n=title;h.textContent=t(title);node.append(h);parent.append(node);return node;}
function move(card,ids){for(const id of ids){const node=$(id);if(node)card.append(node.closest('label')||node);}}
function segment(id){const select=$(id),group=document.createElement('div');group.className='segmented';group.setAttribute('role','group');group.setAttribute('aria-label',select.closest('label').textContent.trim());for(const option of select.options){const b=document.createElement('button');b.type='button';b.dataset.option=option.value;if(option.dataset.i18n)b.dataset.i18n=option.dataset.i18n;else b.textContent=option.textContent;b.onclick=()=>{select.value=option.value;select.dispatchEvent(new Event('change',{bubbles:true}));refreshPresentation();};group.append(b);}select.tabIndex=-1;select.closest('label').setAttribute('aria-hidden','true');select.closest('label').classList.add('control-backed');select.closest('label').after(group);group.dataset.control=id;}
function buildPresentation(){
 for(const name of ['display','usage','weather','stocks','wifi','firmware']){
  const section=$('section-'+name),body=document.createElement('div');body.className='section-body';const editor=document.createElement('div');editor.className='editor-column';const aside=document.createElement('aside');aside.className='saved-column';aside.setAttribute('aria-label',t('Saved on device'));for(const child of [...section.children])if(!child.classList.contains('section-head'))editor.append(child);body.append(editor,aside);section.append(body);const state=card('Saved on device',aside);state.id='saved-'+name;const dirty=document.createElement('span');dirty.className='draft-indicator';dirty.id='draft-'+name;section.querySelector('.section-head').append(dirty);
 }
 const ed=name=>$('section-'+name).querySelector('.editor-column');
 const display=ed('display'),old=$('brightnessPercent').closest('fieldset');
 const base=card('Base brightness',display);base.classList.add('brightness-card');move(base,['brightnessPercent']);move($('saved-display'),['brightnessStored']);
 const schedule=card('Scheduled brightness',display);schedule.classList.add('schedule-card');move(schedule,['brightnessAuto']);schedule.append(document.querySelector('.schedule'));schedule.insertAdjacentHTML('beforeend','<div class="day-track"></div><div class="timeline"><span>00:00</span><span>06:00</span><span>12:00</span><span>18:00</span><span>24:00</span></div><div class="timeline" style="font-size:12px;margin-top:10px"><span id="scheduleDay"></span><span id="scheduleNight"></span></div>');move($('saved-display'),['brightnessScheduleStored','brightnessEffective']);
 const lang=card('Language',display);move(lang,['language','languageStored']);segment('language');
 move(display,['displayStatus']);const adv=display.querySelector('details');if(adv){move(adv,['autoRotateSeconds']);display.append(adv);}old.remove();
 const details=document.createElement('details');details.innerHTML='<summary data-i18n="Data status"></summary>';move(details,['brightnessStored','brightnessScheduleStored','brightnessEffective']);$('saved-display').append(details);const device=$('saved-display');device.querySelector('h3').insertAdjacentHTML('afterend','<strong class="hero-value" id="displayPreview">—</strong><p id="displayPreviewSchedule"></p>');
 const usage=ed('usage'),oldUsage=usage.querySelector('fieldset');const services=card('Service selection',usage);services.append(document.querySelector('.provider-identities'));
 for(const name of ['claude','codex']){const meters=document.createElement('div');meters.id=name+'Meters';meters.className='usage-meters';$(name+'Identity').parentElement.append(meters);}
 const mode=card('Codex window',usage);move(mode,['codexUsageMode','displayCurrent']);segment('codexUsageMode');mode.insertAdjacentHTML('beforeend','<div id="modePreview"></div>');
 const hook=card('Data source',usage);hook.insertAdjacentHTML('beforeend','<small data-i18n="Usage hook"></small><code>POST /api/usage</code>');move(hook,['usageTokenStored','usageToken']);oldUsage.remove();
 const weather=ed('weather'),oldWeather=$('weatherUrl').closest('fieldset');const location=card('Location',weather);move(location,['weatherLabel']);const pair=document.createElement('div');pair.className='paired';move(pair,['weatherLatitude','weatherLongitude']);location.append(pair);
 const source=card('Data source',weather),sourcePair=document.createElement('div');sourcePair.className='paired';move(sourcePair,['weatherProvider','weatherRefresh']);source.append(sourcePair);const weatherAdvanced=oldWeather.querySelector('details');if(weatherAdvanced)source.append(weatherAdvanced);
 const status=card('Data status',weather);move(status,['weatherDataStatus','networkRefresh']);const weatherImage=$('weatherArtwork');weatherImage.remove();oldWeather.remove();
 const hero=card('Weather preview',$('section-weather').querySelector('aside'));hero.classList.add('weather-hero');hero.append(weatherImage);hero.insertAdjacentHTML('beforeend','<svg id="neutralWeather" class="neutral-weather" viewBox="0 0 100 90" aria-hidden="true" hidden><circle cx="50" cy="43" r="29" fill="none" stroke="currentColor" stroke-width="3"/><text x="50" y="54" text-anchor="middle" fill="currentColor" font-size="32">?</text></svg><strong class="hero-name" id="weatherPlace"></strong><strong class="hero-value" id="weatherTemperature">—</strong><p id="weatherCondition"></p><span class="hero-meta" id="weatherHumidity"></span><span class="hero-meta" id="weatherSource"></span>');
 const stock=ed('stocks'),oldStock=$('stocksProvider').closest('fieldset'),stockSource=card('Data source',stock),stockPair=document.createElement('div');stockPair.className='paired';move(stockPair,['stocksProvider','stocksRefresh']);stockSource.append(stockPair);
 const rangeControl=$('stocksRange').closest('label');const watch=card('รายการหุ้น',stock);watch.insertAdjacentHTML('beforeend','<div class="watch-head"><span data-i18n="Market"></span><span data-i18n="Ticker"></span><span data-i18n="Show"></span></div>');for(const row of [...oldStock.querySelectorAll('.stock-editor')])watch.append(row);move(watch,['stocksSelectionStored']);const hint=document.createElement('small');hint.dataset.i18n='หุ้นไทยใส่ชื่อย่อ เช่น PTT เลือก TH; US เช่น AAPL ว่างเพื่อลบรายการ';watch.append(hint);for(const detail of [...oldStock.querySelectorAll('details')])stock.append(detail);oldStock.remove();
 const quote=card('Quote preview',$('section-stocks').querySelector('aside'));quote.classList.add('quote-preview');quote.insertAdjacentHTML('beforeend','<strong class="hero-name" id="quoteSymbol">—</strong><strong class="hero-value" id="quotePrice">—</strong><span class="badge" id="quoteChange"></span><svg id="quoteChart" class="quote-chart" viewBox="0 0 240 75" aria-label="Actual price history"><path id="quotePath"/></svg><small id="quoteStatus"></small>');quote.append(rangeControl);segment('stocksRange');
 const wifi=ed('wifi'),oldWifi=$('ssid').closest('fieldset');const connection=card('Current connection',wifi);connection.insertAdjacentHTML('beforeend','<div class="connection" id="connectionName">—</div><span class="badge" id="connectionState"></span>');move(connection,['ssidStored']);const edit=card('Edit Wi-Fi network',wifi);move(edit,['ssid','wifiPasswordStored','wifiPassword']);const advanced=card('Advanced',wifi);move(advanced,['clearSsid','clearWifi']);oldWifi.remove();
 const fw=ed('firmware'),oldFw=$('firmwareFile').closest('fieldset');const info=card('Device information',fw);move(info,['firmwareInfo']);const upload=card('Upload firmware',fw);upload.append($('firmwareFile'),document.querySelector('[for=firmwareFile]'));move(upload,['firmwareSelected','firmwareReview','firmwareButton','firmwareProgress','firmwareStatus']);oldFw.remove();$('firmwareProgress').hidden=true;$('saved-firmware').hidden=true;$('draft-firmware').hidden=true;const remove=document.createElement('button');remove.type='button';remove.id='firmwareRemove';remove.className='secondary';remove.dataset.i18n='Remove file';remove.hidden=true;remove.onclick=()=>{$('firmwareFile').value='';$('firmwareFile').dispatchEvent(new Event('change',{bubbles:true}));};$('firmwareSelected').after(remove);$('firmwareFile').addEventListener('change',()=>{remove.hidden=!$('firmwareFile').files.length;});
 for(const locale of ['TH','EN']){$('header'+locale).title=t('Portal language only');$('header'+locale).onclick=()=>applyLanguage(locale.toLowerCase());}
 refreshPresentation();
}
function meters(node,provider,weeklyOnly=false){
 node.replaceChildren();if(!provider?.hasData){node.textContent=t('No live data');node.className='usage-meters meter-unknown';return;}
 node.className='usage-meters';for(const [key,label] of [['primary','Last 5 hours'],['secondary','Weekly total']]){if(weeklyOnly&&key==='primary')continue;const w=provider[key],value=w?.usedPercent;const row=document.createElement('div');row.className='meter-row';const caption=document.createElement('div');caption.className='meter-caption';const title=document.createElement('span');title.textContent=Number.isFinite(w?.windowMinutes)&&w.windowMinutes!==(key==='primary'?300:10080)?w.windowMinutes+' min':t(label);const number=document.createElement('strong');number.textContent=Number.isFinite(value)?value+'%':'—';caption.append(title,number);const meter=document.createElement('div');meter.className='meter';if(Number.isFinite(value)){const fill=document.createElement('i');fill.style.width=Math.max(0,Math.min(100,value))+'%';meter.append(fill);}row.append(caption,meter);node.append(row);}if(provider.stale){const stale=document.createElement('small');stale.textContent=t('Stale');node.append(stale);}
}
function refreshPresentation(){
 if(!document.body||!$('displayPreview'))return;
 for(const [id,key] of [['sectionSelect','Section'],['brightnessPercent','ความสว่าง'],['brightnessAuto','ความสว่างอัตโนมัติ'],['touch','Touch calibration']])$(id).setAttribute('aria-label',t(key));
 for(let i=0;i<5;i++)$('stockEnabled'+i).setAttribute('aria-label',t('Show')+' '+$('stockMarket'+i).value+' '+$('stockSymbol'+i).value.trim()+' · '+t('Row')+' '+(i+1));
 for(const group of document.querySelectorAll('[data-control]'))for(const button of group.children)button.setAttribute('aria-pressed',String(button.dataset.option===$(group.dataset.control).value));
 const brightness=Number($('brightnessPercent').value);$('scheduleDay').textContent=t('กลางวัน')+' '+brightness+'%';$('scheduleNight').textContent=t('กลางคืน')+' '+Math.max(5,Math.floor(brightness/2))+'%';const track=document.querySelector('.day-track');if(track){const hour=id=>{const v=$(id).value.split(':').map(Number);return (v[0]*60+v[1])/14.4;},day=hour('brightnessDayStart'),night=hour('brightnessNightStart');track.style.background=day<night?'linear-gradient(90deg,#4c5a63 '+day+'%,#f5e7c6 '+day+'%,#f5e7c6 '+night+'%,#4c5a63 '+night+'%)':'linear-gradient(90deg,#f5e7c6 '+night+'%,#4c5a63 '+night+'%,#4c5a63 '+day+'%,#f5e7c6 '+day+'%)';}
 $('displayPreview').textContent=displaySaved?displaySaved.brightnessPercent+'%':'—';$('displayPreviewSchedule').textContent=displaySaved?displaySaved.brightnessDayStart+' – '+displaySaved.brightnessNightStart+' · '+displaySaved.language:t('No live data');
 for(const name of ['claude','codex'])meters($(name+'Meters'),dashboard?.[name]);meters($('modePreview'),dashboard?.codex,$('codexUsageMode').value==='weekly');
 const stock=dashboard?.stock;const valid=stock?.hasData;$('quoteSymbol').textContent=valid?stock.symbol:t('No live data');$('quotePrice').textContent=valid&&Number.isFinite(stock.price)?stock.price.toFixed(2):'—';$('quoteChange').style.color=valid&&Number.isFinite(stock.changePercent)&&stock.changePercent<0?'#F5874B':'#8AE4CC';$('quoteChange').textContent=valid&&Number.isFinite(stock.changePercent)?(stock.changePercent>0?'+':'')+stock.changePercent.toFixed(2)+'% · '+(stock.currency||''):'';
 const history=valid&&Array.isArray(stock.history)?stock.history.slice(-32):[],points=history.filter(Number.isFinite);$('quoteChart').toggleAttribute('hidden',points.length<2);let d='';if(points.length>1){const low=Math.min(...points),range=Math.max(...points)-low||1;history.forEach((n,i)=>{if(!Number.isFinite(n)){d+=' ';return;}d+=(i===0||!Number.isFinite(history[i-1])?'M':'L')+(i*240/Math.max(1,history.length-1)).toFixed(1)+','+(68-(n-low)*60/range).toFixed(1);});}$('quotePath').setAttribute('d',d);$('quoteStatus').textContent=valid?(stock.stale?t('Stale'):t('Actual received data'))+' · '+(['1d','5d'].includes(stock.chartRange)?stock.chartRange:'—')+(points.length<2?' · '+t('No price history'):''):t('No live data');
 const w=lastNetwork?.weather,has=w?.hasData;$('weatherPlace').textContent=saved?.weather?.label||t('Location');$('weatherTemperature').textContent=has&&Number.isFinite(w.temperature)?w.temperature.toFixed(1)+'°C':'—';const c=w?.weatherCode;let condition=has?(c===0||c===1?'Clear':c===2?'Partly cloudy':c===3||c===45||c===48?'Cloudy':c>=51?'Rain / precipitation':'No live data'):'No live data';$('weatherCondition').textContent=t(condition)+(has&&w.stale?' · '+t('Stale'):'');$('weatherHumidity').textContent=t('Humidity')+': '+(has&&Number.isFinite(w.humidity)?w.humidity+'%':'—');const unknownDay=has&&[0,1,2].includes(c)&&typeof w.isDay!=='boolean';$('neutralWeather').toggleAttribute('hidden',!unknownDay||c===2);if(unknownDay){if(c===2){$('weatherArtwork').src=portalImages.weather4;$('weatherArtwork').hidden=false;$('weatherArtwork').dataset.tile='4';}else $('weatherArtwork').hidden=true;}$('weatherSource').textContent=(saved?.weather?.url?.includes('api.open-meteo.com')?'Open-Meteo':t('Custom endpoint'))+(unknownDay?' · '+t('Unknown day / night'):'');
 $('connectionName').textContent=saved?.currentSSID||t('No live data');$('connectionState').textContent=typeof lastNetwork?.stationOnline==='boolean'?t(lastNetwork.stationOnline?'Connected':'Offline'):t('ไม่ทราบ');
}
async function loadDashboard(){if(dashboardLoading)return;dashboardLoading=true;const controller=new AbortController(),timeout=setTimeout(()=>controller.abort(),2000);try{const response=await fetch('/api/dashboard',{cache:'no-store',signal:controller.signal});if(!response.ok)throw Error();const value=await response.json();dashboard=value&&typeof value==='object'?value:null;}catch(_){dashboard=null;}finally{clearTimeout(timeout);dashboardLoading=false;refreshPresentation();}}
if(document.body){document.addEventListener('input',refreshPresentation);document.addEventListener('change',refreshPresentation);}
if(document.body)setupSections();
</script></html>)HTML";
}
