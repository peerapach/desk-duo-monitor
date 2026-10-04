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
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Desk Duo setup</title>
<style>body{font:16px system-ui;max-width:680px;margin:auto;padding:20px;background:#f3f0e7;color:#292b2d}fieldset{margin:18px 0;border:1px solid #c9c6bf;border-radius:8px}label{display:block;margin:10px 0}input,textarea,button{font:inherit;box-sizing:border-box;max-width:100%}input:not([type=checkbox]),textarea{display:block;width:100%;padding:8px}textarea{height:84px}small{overflow-wrap:anywhere}button{padding:10px 20px}#status{white-space:pre-wrap}</style>
<h1>Desk Duo</h1><p>ตั้งค่า Wi-Fi และแหล่งข้อมูลของจอ ค่าลับที่เว้นว่างจะคงค่าเดิม</p>
<form id="settings" autocomplete="off">
<fieldset><legend>Wi-Fi / usage</legend>
<label>แทนที่ Wi-Fi SSID (ว่าง = คงค่าเดิม)<input id="ssid" maxlength="32"></label><label><input id="clearSsid" type="checkbox"> ปิดการเชื่อมต่อ Wi-Fi</label>
<label>เปลี่ยนรหัส Wi-Fi<input id="wifiPassword" type="password" maxlength="64" autocomplete="new-password"></label>
<label><input id="clearWifi" type="checkbox"> ล้างรหัส Wi-Fi (เครือข่ายเปิด)</label>
<label>เปลี่ยน Usage bearer token<input id="usageToken" type="password" minlength="16" maxlength="128" autocomplete="new-password"></label>
<label>หมุนหน้าอัตโนมัติ (0 ปิด หรือ 5–300 วินาที)<input id="autoRotateSeconds" type="number" min="0" max="300"></label></fieldset>
<div id="sources"></div>
<fieldset><legend>Touch calibration</legend><p>ตรวจมุมจอจริงก่อนแก้ ค่า JSON ใช้ swapAxes, invertX, invertY, minX, maxX, minY, maxY</p><textarea id="touch" spellcheck="false"></textarea></fieldset>
<button>บันทึก</button><p id="status" role="status"></p></form>
<script>
const $=id=>document.getElementById(id);
for(const name of ['weather','stocks']){
 const box=document.createElement('fieldset');
 box.innerHTML=`<legend>${name==='weather'?'อากาศ':'หุ้นจาก Google Sheets'}</legend>
 <small id="${name}Stored"></small>
 <label>แทนที่ URL (ว่าง = คงค่าเดิม)<input id="${name}Url" type="url" maxlength="1024" placeholder="https://..."></label>
 <label><input id="${name}Clear" type="checkbox"> ปิดแหล่งข้อมูล / ล้าง URL</label>
 <label>แทนที่ headers เป็น JSON (ว่าง = คงค่าเดิม, {} = ล้าง)<textarea id="${name}Headers" spellcheck="false" placeholder='{"Authorization":"Bearer ..."}'></textarea></label>
 <label>Refresh วินาที (30–86400)<input id="${name}Refresh" type="number" min="30" max="86400"></label>`;
 if(name==='weather')box.innerHTML+='<label>ชื่อสถานที่<input id="weatherLabel" maxlength="64"></label><label>Field mappings (JSON paths)<textarea id="weatherFields" spellcheck="false"></textarea></label>';
 $('sources').append(box);
}
let saved;
async function load(){
 const r=await fetch('/api/config',{cache:'no-store'});if(!r.ok)throw Error('โหลดการตั้งค่าไม่สำเร็จ');
 saved=await r.json();$('ssid').value='';$('clearSsid').checked=false;$('autoRotateSeconds').value=saved.autoRotateSeconds;$('touch').value=JSON.stringify(saved.touch,null,2);
 for(const n of ['weather','stocks']){
  $(n+'Stored').textContent='URL ปัจจุบัน (ซ่อน query): '+(saved[n].url||'ยังไม่ตั้ง');
  $(n+'Url').value='';$(n+'Headers').value='';$(n+'Clear').checked=false;$(n+'Refresh').value=saved[n].refreshSeconds;
 }
 $('weatherLabel').value=saved.weather.label;$('weatherFields').value=JSON.stringify(saved.weather.fields,null,2);
 $('wifiPassword').value='';$('usageToken').value='';$('clearWifi').checked=false;
}
$('settings').addEventListener('submit',async e=>{
 e.preventDefault();$('status').textContent='กำลังบันทึก...';
 try{
  if(!saved)throw Error('รอโหลดการตั้งค่า');
  const patch={autoRotateSeconds:Number($('autoRotateSeconds').value),touch:JSON.parse($('touch').value)};
  if($('clearSsid').checked)patch.ssid='';else if($('ssid').value)patch.ssid=$('ssid').value;
  if($('clearWifi').checked)patch.wifiPassword='';else if($('wifiPassword').value)patch.wifiPassword=$('wifiPassword').value;
  if($('usageToken').value)patch.usageToken=$('usageToken').value;
  for(const n of ['weather','stocks']){
   patch[n]={refreshSeconds:Number($(n+'Refresh').value)};
   // Never round-trip GET's redacted URL or any hidden secret.
   if($(n+'Clear').checked)patch[n].url='';else if($(n+'Url').value)patch[n].url=$(n+'Url').value;
   if($(n+'Headers').value.trim())patch[n].headers=JSON.parse($(n+'Headers').value);
  }
  patch.weather.label=$('weatherLabel').value;patch.weather.fields=JSON.parse($('weatherFields').value);
  const r=await fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(patch)});
  if(!r.ok)throw Error(r.status===503?'บันทึกลง flash ไม่สำเร็จ ยังไม่เปลี่ยนค่า':'ตรวจสอบค่า / สิทธิ์ (HTTP '+r.status+')');
  await load();$('status').textContent='บันทึกแล้ว Wi-Fi จะเชื่อมต่อใหม่เมื่อเปลี่ยนค่า';
 }catch(err){$('status').textContent=err.message;}
});load().catch(e=>$('status').textContent=e.message);
</script></html>)HTML";
}
