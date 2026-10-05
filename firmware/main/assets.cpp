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
<style>body{font:16px system-ui;max-width:680px;margin:auto;padding:20px;background:#000;color:#e9edf2}fieldset{margin:18px 0;border:1px solid #363a40;border-radius:8px}label{display:block;margin:10px 0}input,textarea,select,button{font:inherit;box-sizing:border-box;max-width:100%}input:not([type=checkbox]),textarea{display:block;width:100%;padding:8px}textarea{height:84px}small{overflow-wrap:anywhere}button{padding:10px 20px}#status,#firmwareStatus{white-space:pre-wrap}input,textarea,select{background:#14171b;color:#e9edf2;border:1px solid #515861;border-radius:6px}button{background:#a5c9ee;color:#061322;border:0;border-radius:8px}button:disabled{opacity:.5}progress{width:100%;accent-color:#a5c9ee}</style>
<h1>Desk Duo</h1><p>ตั้งค่า Wi-Fi และแหล่งข้อมูลของจอ ค่าลับที่เว้นว่างจะคงค่าเดิม</p>
<form id="settings" autocomplete="off">
<fieldset><legend>Wi-Fi / usage</legend>
<small id="ssidStored">SSID ที่บันทึก: กำลังโหลด...</small>
<label>แก้ไข Wi-Fi SSID (ว่าง = คงค่าเดิม)<input id="ssid" maxlength="32"></label><label><input id="clearSsid" type="checkbox"> ปิดการเชื่อมต่อ Wi-Fi</label>
<small id="wifiPasswordStored">รหัส Wi-Fi ปัจจุบัน: กำลังโหลด...</small>
<label>เปลี่ยนรหัส Wi-Fi<input id="wifiPassword" type="password" maxlength="64" autocomplete="new-password"></label>
<label><input id="clearWifi" type="checkbox"> ล้างรหัส Wi-Fi (เครือข่ายเปิด)</label>
<small id="usageTokenStored">Usage token ปัจจุบัน: กำลังโหลด...</small>
<label>เปลี่ยน Usage bearer token<input id="usageToken" type="password" minlength="16" maxlength="128" autocomplete="new-password"></label>
<label>หมุนหน้าอัตโนมัติ (0 ปิด หรือ 5–300 วินาที)<input id="autoRotateSeconds" type="number" min="0" max="300"></label></fieldset>
<div id="sources"></div>
<fieldset><legend>Touch calibration</legend><p>ตรวจมุมจอจริงก่อนแก้ ค่า JSON ใช้ swapAxes, invertX, invertY, minX, maxX, minY, maxY</p><textarea id="touch" spellcheck="false"></textarea></fieldset>
<button id="settingsButton" type="submit" disabled>บันทึก</button><p id="status" role="status"></p></form>
<form id="display"><fieldset><legend>การแสดง usage ของ Codex</legend>
<label>แสดงช่วง usage<select id="codexUsageMode"><option value="both">5h + Weekly</option><option value="weekly">Weekly อย่างเดียว</option></select></label>
<button id="displayButton" type="submit" disabled>บันทึกการแสดง usage</button><p id="displayStatus" role="status"></p>
</fieldset></form>
<form id="firmware"><fieldset><legend>Firmware update</legend>
<p id="firmwareInfo">Loading firmware status...</p>
<p>เลือก desk-duo-app.bin สำหรับอัปเดตผ่าน Wi-Fi การตั้งค่าจะคงอยู่</p>
<input id="firmwareFile" type="file" accept=".bin,application/octet-stream" required>
<button id="firmwareButton" type="submit">Upload firmware</button>
<progress id="firmwareProgress" max="100" value="0"></progress><p id="firmwareStatus" role="status"></p>
</fieldset></form>
<script>
const $=id=>document.getElementById(id);
for(const name of ['weather','stocks']){
 const box=document.createElement('fieldset');
 box.innerHTML=`<legend>${name==='weather'?'อากาศ':'หุ้นจาก Google Sheets'}</legend>
 <small id="${name}Stored">URL ที่บันทึก: กำลังโหลด...</small>
 <label>แทนที่ URL (ว่าง = คงค่าเดิม)<input id="${name}Url" type="url" maxlength="1024" placeholder="https://..."></label>
 <label><input id="${name}Clear" type="checkbox"> ปิดแหล่งข้อมูล / ล้าง URL</label>
 <small id="${name}HeadersStored">Headers ปัจจุบัน: กำลังโหลด...</small>
 <label>แทนที่ headers เป็น JSON (ว่าง = คงค่าเดิม, {} = ล้าง)<textarea id="${name}Headers" spellcheck="false" placeholder='{"Authorization":"Bearer ..."}'></textarea></label>
 <label>Refresh วินาที (30–86400)<input id="${name}Refresh" type="number" min="30" max="86400"></label>`;
 if(name==='weather')box.innerHTML+='<p id="weatherDataStatus" role="status">ข้อมูลอากาศ: กำลังโหลด...</p><button id="networkRefresh" type="button">อัปเดตสถานะ</button><small>อ่านสถานะล่าสุดของเครื่อง การอ่านไม่สั่งดึงข้อมูลใหม่</small><label>ชื่อสถานที่<input id="weatherLabel" maxlength="64"></label><label>Field mappings (JSON paths)<textarea id="weatherFields" spellcheck="false"></textarea></label>';
 $('sources').append(box);
}
let saved;
let saving=false;
const presence=value=>value===true?'ตั้งค่าแล้ว':value===false?'ยังไม่ตั้งค่า':'ไม่ทราบ';
function unknownSettings(){
 saved=undefined;$('settingsButton').disabled=true;
 for(const id of ['ssidStored','wifiPasswordStored','usageTokenStored','weatherStored','stocksStored','weatherHeadersStored','stocksHeadersStored'])$(id).textContent='ค่าปัจจุบัน: ไม่ทราบ (โหลดไม่สำเร็จ)';
}
async function load(){
 saved=undefined;$('settingsButton').disabled=true;
 try{
  const r=await fetch('/api/config',{cache:'no-store'});if(!r.ok)throw Error('โหลดการตั้งค่าไม่สำเร็จ');
  const current=await r.json();
  // Do not permit writes from an incomplete or older redacted response.
  const object=value=>value && typeof value==='object' && !Array.isArray(value);
  if(typeof current.currentSSID!=='string' || typeof current.wifiPasswordConfigured!=='boolean' || typeof current.usageTokenConfigured!=='boolean' || !Number.isFinite(current.autoRotateSeconds) ||
     !object(current.touch) || !['swapAxes','invertX','invertY'].every(key=>typeof current.touch[key]==='boolean') || !['minX','maxX','minY','maxY'].every(key=>Number.isFinite(current.touch[key])) ||
     !object(current.weather?.fields) || !Object.values(current.weather.fields).every(value=>typeof value==='string') || typeof current.weather.label!=='string' ||
     !['weather','stocks'].every(n=>object(current[n]) && typeof current[n].url==='string' && Number.isFinite(current[n].refreshSeconds) && typeof current[n].headersConfigured==='boolean' && typeof current[n].urlDetailsHidden==='boolean'))throw Error('โหลดการตั้งค่าไม่สำเร็จ');
  $('ssid').value=current.currentSSID;$('ssidStored').textContent='SSID ที่บันทึก: '+(current.currentSSID||'ยังไม่ตั้งค่า');
  $('wifiPasswordStored').textContent='รหัส Wi-Fi ปัจจุบัน: '+presence(current.wifiPasswordConfigured);
  $('usageTokenStored').textContent='Usage token ปัจจุบัน: '+presence(current.usageTokenConfigured);
  $('clearSsid').checked=false;$('autoRotateSeconds').value=current.autoRotateSeconds;$('touch').value=JSON.stringify(current.touch,null,2);
  for(const n of ['weather','stocks']){
   $(n+'Stored').textContent='URL ที่บันทึก: '+(current[n].url||'ยังไม่ตั้งค่า')+(current[n].urlDetailsHidden?' (ซ่อน query / fragment)':'');
   $(n+'HeadersStored').textContent='Headers ปัจจุบัน: '+presence(current[n].headersConfigured);
   $(n+'Url').value='';$(n+'Headers').value='';$(n+'Clear').checked=false;$(n+'Refresh').value=current[n].refreshSeconds;
  }
  $('weatherLabel').value=current.weather.label;$('weatherFields').value=JSON.stringify(current.weather.fields,null,2);
  $('wifiPassword').value='';$('usageToken').value='';$('clearWifi').checked=false;
  saved=current;$('settingsButton').disabled=false;
 }catch(err){unknownSettings();throw err;}
}
function editedObject(text,current){
 const value=JSON.parse(text);
 if(!value || typeof value!=='object' || Array.isArray(value))throw Error('ตรวจสอบค่า JSON object');
 return Object.fromEntries(Object.entries(value).filter(([key,item])=>item!==current[key]));
}
$('settings').addEventListener('submit',async e=>{
 e.preventDefault();if(saving)return;
 try{
  if(!saved)throw Error('รอโหลดการตั้งค่า / โหลดหน้าใหม่ก่อนบันทึก');
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
   if($(n+'Headers').value.trim())source.headers=JSON.parse($(n+'Headers').value);
   if(n==='weather'){
    if($('weatherLabel').value!==saved.weather.label)source.label=$('weatherLabel').value;
    const fields=editedObject($('weatherFields').value,saved.weather.fields);if(Object.keys(fields).length)source.fields=fields;
   }
   if(Object.keys(source).length)patch[n]=source;
  }
  if(!Object.keys(patch).length){$('status').textContent='ไม่มีการเปลี่ยนแปลง';return;}
  saving=true;$('settingsButton').disabled=true;$('status').textContent='กำลังบันทึก...';
  const r=await fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(patch)});
  if(!r.ok)throw Error(r.status===503?'Firmware maintenance busy หรือบันทึกไม่สำเร็จ ยังไม่เปลี่ยนค่า กรุณาลองอีกครั้ง':'ตรวจสอบค่า / สิทธิ์ (HTTP '+r.status+')');
  invalidateNetworkStatus();
  // The POST has committed even if fetching its new snapshot fails.
  try{await load();}catch(_){$('status').textContent='บันทึกแล้ว แต่โหลดค่าปัจจุบันไม่สำเร็จ กรุณาโหลดหน้าใหม่ก่อนแก้ไขอีกครั้ง';return;}
  $('status').textContent='บันทึกแล้ว Wi-Fi จะเชื่อมต่อใหม่เมื่อเปลี่ยนค่า';
 }catch(err){$('status').textContent=err.message;}
 finally{saving=false;$('settingsButton').disabled=!saved;}
});load().catch(e=>$('status').textContent=e.message);
let displayLoaded=false;
$('displayButton').disabled=true;
async function loadDisplay(){
 const r=await fetch('/api/display',{cache:'no-store'});if(!r.ok)throw Error('โหลดการแสดง usage ไม่สำเร็จ');
 const current=await r.json();if(!['both','weekly'].includes(current.codexUsageMode))throw Error('โหลดการแสดง usage ไม่สำเร็จ');
 $('codexUsageMode').value=current.codexUsageMode;displayLoaded=true;$('displayButton').disabled=false;
}
$('display').addEventListener('submit',async e=>{
 e.preventDefault();
 try{
  if(!displayLoaded)throw Error('รอโหลดการแสดง usage');
  $('displayButton').disabled=true;$('displayStatus').textContent='กำลังบันทึก...';
  const r=await fetch('/api/display',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({codexUsageMode:$('codexUsageMode').value})});
  if(!r.ok)throw Error(r.status===503?'ยังบันทึกไม่ได้ กรุณาลองอีกครั้ง':'ตรวจสอบค่า / สิทธิ์ (HTTP '+r.status+')');
  $('displayStatus').textContent='บันทึกการแสดง usage แล้ว';
 }catch(err){$('displayStatus').textContent=err.message;}
 finally{$('displayButton').disabled=!displayLoaded;}
});loadDisplay().catch(e=>$('displayStatus').textContent=e.message);
let firmwareInfo;
let updating=false;
async function firmwareStatus(){
 const controller=new AbortController();const timeout=setTimeout(()=>controller.abort(),2000);
 try{
  const r=await fetch('/api/firmware',{cache:'no-store',signal:controller.signal});
  if(!r.ok)throw Error('Firmware status unavailable (HTTP '+r.status+')');
  firmwareInfo=await r.json();
  $('firmwareInfo').textContent='Version '+firmwareInfo.version+' · '+firmwareInfo.capacity+' bytes capacity'+(firmwareInfo.busy?' · Update busy':'');
  return firmwareInfo;
 }finally{clearTimeout(timeout);}
}
function updateFailed(message){updating=false;$('firmwareButton').disabled=false;$('firmwareStatus').textContent=message;}
async function reconnect(previous){
 $('firmwareStatus').textContent='Validated. Restarting device; waiting to reconnect...';
 for(let attempt=0;attempt<30;attempt++){
  await new Promise(resolve=>setTimeout(resolve,2000));
  try{
   const info=await firmwareStatus();
   if(!info.busy && info.runningPartition!==previous){
    updating=false;$('firmwareButton').disabled=false;$('firmwareStatus').textContent='Update complete. Connected to version '+info.version;return;
   }
  }catch(_){}
 }
 updateFailed('Reconnect could not be confirmed. Check the device and reload this page to retry.');
}
$('firmware').addEventListener('submit',async e=>{
 e.preventDefault();if(updating)return;
 try{
  const file=$('firmwareFile').files[0];
  if(!file || !file.name.endsWith('.bin') || file.name==='desk-duo-cyd.bin')throw Error('Choose an application .bin (desk-duo-app.bin).');
  updating=true;$('firmwareButton').disabled=true;
  const info=await firmwareStatus();
  if(info.busy)throw Error('Firmware maintenance busy. Retry shortly.');
  if(!file.size || file.size>info.capacity)throw Error('Application size exceeds slot capacity or file is empty.');
  const req=new XMLHttpRequest();req.open('POST','/api/firmware');req.timeout=130000;
  req.setRequestHeader('Content-Type','application/octet-stream');req.setRequestHeader('X-DeskDuo-Update','1');
  $('firmwareProgress').value=0;$('firmwareStatus').textContent='Uploading... keep the device powered.';
  req.upload.onprogress=e=>{if(e.lengthComputable){$('firmwareProgress').value=100*e.loaded/e.total;if(e.loaded===e.total)$('firmwareStatus').textContent='Upload sent. Verifying application...';}};
  req.onerror=req.ontimeout=req.onabort=()=>updateFailed('Upload interrupted. Check device status and retry.');
  req.onload=()=>{if(req.status===200)return reconnect(info.runningPartition);updateFailed(req.status===503?'Firmware maintenance busy. Retry shortly.':'Update rejected (HTTP '+req.status+'). Check the application file and retry.');};
  req.send(file);
 }catch(err){updateFailed(err.message);}
});
function weatherStatusText(info){
 const w=info.weather,a=info.attempt;
 if(typeof info.stationOnline!=='boolean' || typeof info.weatherConfigured!=='boolean' || !w || typeof w.hasData!=='boolean' || !a || typeof a.attempted!=='boolean')throw Error('invalid status');
 let state;
 if(!info.weatherConfigured)state='ยังไม่ตั้งค่าแหล่งข้อมูล';
 else if(!info.stationOnline)state='เครือข่ายยังไม่เชื่อมต่อ';
 else if(a.attempted && !a.currentGeneration)state='รอดึงข้อมูลสำหรับการตั้งค่าปัจจุบัน';
 else if(!a.attempted || a.outcome==='running')state='รอดึงข้อมูล';
 else if(a.outcome==='clock_wait')state='รอเวลาของเครื่องก่อนเชื่อมต่อ HTTPS';
 else if(a.outcome==='accepted')state='รับข้อมูลแล้ว';
 else if(a.outcome==='parse_failure')state='รูปแบบข้อมูลไม่ถูกต้อง';
 else if(a.outcome==='http_status')state='แหล่งข้อมูลตอบ HTTP '+(Number.isInteger(a.httpStatus)?a.httpStatus:'ไม่ทราบ');
 else if(a.outcome==='tls_failure')state='เชื่อมต่อ HTTPS ไม่สำเร็จ';
 else state='เครือข่าย / การดึงข้อมูลไม่สำเร็จ';
 const values=[];
 if(w.hasData){
  for(const [key,unit] of [['temperature','°C'],['humidity','%']])if(Number.isFinite(w[key]))values.push(w[key]+unit);
  if(Number.isInteger(w.weatherCode))values.push('รหัสอากาศ '+w.weatherCode);
  if(Number.isInteger(w.hourlyCount))values.push('รายชั่วโมง '+w.hourlyCount);
  if(Number.isFinite(w.ageMs))values.push('อายุ '+Math.floor(w.ageMs/1000)+' วินาที');
 }
 return 'ข้อมูลอากาศ: '+(w.hasData?(w.stale?'ข้อมูลเก่า':'มีข้อมูล')+(values.length?' · '+values.join(' · '):''):'ยังไม่มีข้อมูล')+' · '+state;
}
let networkLoading=false,networkRevision=0;
function invalidateNetworkStatus(){
 networkRevision++;$('weatherDataStatus').textContent='ข้อมูลอากาศ: รออ่านสถานะสำหรับการตั้งค่าปัจจุบัน';
}
async function loadNetwork(){
 if(networkLoading || updating)return;
 networkLoading=true;const revision=networkRevision;$('networkRefresh').disabled=true;
 const controller=new AbortController();const timeout=setTimeout(()=>controller.abort(),2000);
 try{
  const r=await fetch('/api/network',{cache:'no-store',signal:controller.signal});if(!r.ok)throw Error('status failed');
  const text=weatherStatusText(await r.json());if(revision===networkRevision)$('weatherDataStatus').textContent=text;
 }catch(_){if(revision===networkRevision)$('weatherDataStatus').textContent='ข้อมูลอากาศ: ไม่ทราบ (อ่านสถานะไม่สำเร็จ กรุณาลองอีกครั้ง)';}
 finally{clearTimeout(timeout);networkLoading=false;$('networkRefresh').disabled=false;}
}
$('networkRefresh').addEventListener('click',loadNetwork);
loadNetwork();
firmwareStatus().catch(e=>$('firmwareInfo').textContent=e.message);
</script></html>)HTML";
}
