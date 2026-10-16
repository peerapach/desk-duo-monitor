const {test} = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const asset = fs.readFileSync(path.join(__dirname, '../firmware/main/assets.cpp'), 'utf8');
const production = asset.split('<script>')[1].split('</script>')[0];

async function portal(options={}) {
  if(typeof options==='boolean')options={displayLoad:options};
  const scenario=options;
  const {displayLoad=true,configLoad=true,reloadLoad=true}=options;let configLoads=0;let displayLoads=0;let display={codexUsageMode:"weekly",language:options.language||"th",brightnessPercent:100,brightnessAuto:false,brightnessDayStart:"06:00",brightnessNightStart:"18:00",brightnessTargetPercent:100,brightnessEffectivePercent:null,brightnessPhase:"manual",...options.display};
  const uploads=[];let firmware={version:"2.0.0",capacity:1966080,busy:false,runningPartition:"ota_0"};
  const timers=new Map(),displayTimers=new Map();let nextTimer=0;
  const elements = new Map();const translationNodes=[];
  const documentState={documentElement:{lang:"th"},title:""};
  const markup=[];
  const register = html => {
    markup.push(html);
    for(const match of html.matchAll(/data-i18n="([^"]+)"/g))translationNodes.push({dataset:{i18n:match[1]},textContent:''});
    for(const match of html.matchAll(/id="([^"]+)"/g))elements.set(match[1], {
      value:'',checked:false,textContent:'',disabled:false,
      append(element){register(element.innerHTML);},
      addEventListener(name,handler){this[name]=handler;}
    });
  };
  register(asset.split('<script>')[0]);
  const get = id => elements.get(id) || null;
  const settings = {currentSSID:'saved-network',wifiPasswordConfigured:true,usageTokenConfigured:true,autoRotateSeconds:20, touch:{swapAxes:false,invertX:false,invertY:false,minX:0,maxX:4095,minY:0,maxY:4095},
    weather:{url:'https://weather.test/data',headersConfigured:true,urlDetailsHidden:true,label:'Bangkok',fields:{temperature:'current.temperature_2m'},refreshSeconds:900},
    stocks:{url:'https://script.test/exec',headersConfigured:false,urlDetailsHidden:false,refreshSeconds:300},...options.settings};
  const requests = [];const displayRequests=[];const networkRequests=[];const networkLoad=options.networkLoad,networkStatus=options.networkStatus,networkReply=options.networkReply;let network=options.network||{stationOnline:true,weatherConfigured:true,currentGeneration:7,weather:{hasData:false,unknown:true,stale:false,ageMs:null,temperature:null,humidity:null,weatherCode:null,hourlyCount:0},attempt:{attempted:false,currentGeneration:true,outcome:'not_attempted',httpStatus:null}};
  const context = vm.createContext({
    setTimeout(fn,delay){const id=++nextTimer;(delay===15000?displayTimers:timers).set(id,fn);return id;},clearTimeout(id){timers.delete(id);displayTimers.delete(id);},AbortController,
    XMLHttpRequest:class {constructor(){this.upload={};this.headers={};uploads.push(this);}open(method,url){this.method=method;this.url=url;}setRequestHeader(k,v){this.headers[k]=v;}send(file){this.file=file;}},
    document:{...documentState,getElementById:get,querySelectorAll(){return translationNodes;},createElement(){return {innerHTML:''};}},
    fetch:async (url, options) => {
      if(url==='/api/network'){networkRequests.push(options);if(options.method)throw Error('status must be read only');if(networkReply)return await networkReply();if(networkLoad===false)throw Error('network private error');return {ok:networkStatus!==503,status:networkStatus||200,json:async()=>structuredClone(network)};}
      if(url==='/api/display'){if(!options.method){if(scenario.displayReply)return await scenario.displayReply();if(!displayLoad || (++displayLoads>1 && scenario.displayReload===false))throw Error('offline');return {ok:true,json:async()=>structuredClone(display)};}const patch=JSON.parse(options.body);displayRequests.push(patch);if(scenario.displaySave!==false)Object.assign(display,patch);return {ok:scenario.displaySave!==false,status:scenario.displaySave===false?503:204};}
      if(url==='/api/firmware')return {ok:true,status:200,json:async()=>structuredClone(firmware)};
      if(url!=='/api/config')throw Error('unexpected URL '+url);
      if(options.method){
        const patch=JSON.parse(options.body);requests.push(patch);
        if(Object.hasOwn(patch,'ssid'))settings.currentSSID=patch.ssid;
        if(Object.hasOwn(patch,'wifiPassword'))settings.wifiPasswordConfigured=Boolean(patch.wifiPassword);
        if(Object.hasOwn(patch,'usageToken'))settings.usageTokenConfigured=Boolean(patch.usageToken);
        if(Object.hasOwn(patch,'autoRotateSeconds'))settings.autoRotateSeconds=patch.autoRotateSeconds;
        if(patch.touch)Object.assign(settings.touch,patch.touch);
        for(const source of ['weather','stocks'])if(patch[source]){
          const replacement={...patch[source]};
          if(Object.hasOwn(replacement,'url')){
            settings[source].urlDetailsHidden=/[?#]/.test(replacement.url);
            replacement.url=replacement.url.split(/[?#]/)[0];
          }
          if(Object.hasOwn(replacement,'headers')){
            settings[source].headersConfigured=Object.keys(replacement.headers).length>0;delete replacement.headers;
          }
          Object.assign(settings[source],replacement);
        }
        return {ok:true,status:204};
      }
      if(++configLoads===1?!configLoad:!reloadLoad)throw Error('config offline');
      return {ok:true,status:200,json:async()=>structuredClone(settings)};
    }});
  vm.runInContext(production,context);
  await new Promise(resolve=>setImmediate(resolve));
  const submit = async () => {await get('settings').submit({preventDefault(){}}); return requests.at(-1);};
  return {get,submit,requests,markup,refreshDisplay:()=>vm.runInContext("refreshDisplay()",context),setDisplay(value){display=value;},displaySnapshot:()=>structuredClone(display),scenario,async pollDisplay(){const [id,fn]=displayTimers.entries().next().value;displayTimers.delete(id);await fn();},displayTimers,translationNodes,document:context.document,displayRequests,networkRequests,setNetwork(value){network=value;},networkRefresh:()=>get('networkRefresh').click(),displaySubmit:()=>get("display").submit({preventDefault(){}}),uploads,timers,setFirmware(value){firmware=value;},firmwareSubmit:()=>get("firmware").submit({preventDefault(){}})};
}

test('unchanged settings never POST and changed source fields omit hidden values',{timeout:30000},async()=>{
  const p=await portal();
  await p.submit();assert.equal(p.requests.length,0);
  assert.match(p.get('status').textContent,/ไม่มีการเปลี่ยนแปลง/);
  p.get('weatherLabel').value='Forecast';
  assert.deepEqual(await p.submit(),{weather:{label:'Forecast'}});
});

// Catches redacted settings being mistaken for empty replacements or inserted as HTML.
test('saved values and presence are visible while secret and URL editors stay blank',{timeout:30000},async()=>{
  const marker='<img src=x onerror=alert(1)>';
  const p=await portal({settings:{currentSSID:marker}});
  assert.equal(p.get('ssid').value,marker);
  assert.match(p.get('ssidStored').textContent,/SSID.*บันทึก/);
  assert.ok(p.get('ssidStored').textContent.includes(marker));
  assert.match(p.get('wifiPasswordStored').textContent,/ตั้งค่าแล้ว/);
  assert.match(p.get('usageTokenStored').textContent,/ตั้งค่าแล้ว/);
  assert.match(p.get('weatherHeadersStored').textContent,/ตั้งค่าแล้ว/);
  assert.match(p.get('stocksHeadersStored').textContent,/ยังไม่ตั้งค่า/);
  assert.match(p.get('weatherStored').textContent,/https:\/\/weather.test\/data/);
  assert.match(p.get('weatherStored').textContent,/ซ่อน query.*fragment/);
  assert.doesNotMatch(p.get('stocksStored').textContent,/ซ่อน query/);
  for(const id of ['wifiPassword','usageToken','weatherUrl','stocksUrl','weatherHeaders','stocksHeaders'])assert.equal(p.get(id).value,'',id);
  assert.equal(p.get('autoRotateSeconds').value,20);
  assert.equal(JSON.parse(p.get('touch').value).maxX,4095);
  assert.equal(p.get('weatherRefresh').value,900);
  assert.equal(p.get('weatherLabel').value,'Bangkok');
  assert.deepEqual(JSON.parse(p.get('weatherFields').value),{temperature:'current.temperature_2m'});
  assert.ok(p.markup.every(html=>!html.includes(marker)));
});

test('empty configuration is displayed as unset only after successful loading',{timeout:30000},async()=>{
 const p=await portal({settings:{currentSSID:'',wifiPasswordConfigured:false,usageTokenConfigured:false,
   weather:{url:'',headersConfigured:false,urlDetailsHidden:false,refreshSeconds:900,label:'Weather',fields:{temperature:'current.temperature_2m'}}}});
 for(const id of ['ssidStored','wifiPasswordStored','usageTokenStored','weatherStored','weatherHeadersStored']){assert.ok(p.get(id),'missing current-state control '+id);assert.match(p.get(id).textContent,/ยังไม่ตั้ง/);}
 assert.equal(p.get('settingsButton').disabled,false);
});

test('only actual scalar, mapping and calibration edits are posted',{timeout:30000},async()=>{
 const p=await portal();
 p.get('touch').value=JSON.stringify({maxY:4095,minY:0,maxX:4095,minX:0,invertY:false,invertX:false,swapAxes:false});
 p.get('weatherFields').value=' { "temperature" : "current.temperature_2m" } ';
 await p.submit();assert.equal(p.requests.length,0);
 p.get('autoRotateSeconds').value='30';p.get('stocksRefresh').value='600';
 p.get('weatherFields').value='{"temperature":"data.temp"}';
 const touch=JSON.parse(p.get('touch').value);touch.invertX=true;p.get('touch').value=JSON.stringify(touch);
 assert.deepEqual(await p.submit(),{autoRotateSeconds:30,stocks:{refreshSeconds:600},weather:{fields:{temperature:'data.temp'}},touch:{invertX:true}});
 assert.match(p.get('weatherStored').textContent,/ซ่อน query/);
 await p.submit();assert.equal(p.requests.length,1);
});

test('failed initial config load blocks settings writes while display remains usable',{timeout:30000},async()=>{
 const p=await portal({configLoad:false});
 assert.ok(p.get('settingsButton'),'missing guarded settings Save');assert.equal(p.get('settingsButton').disabled,true);
 for(const id of ['ssidStored','wifiPasswordStored','usageTokenStored','weatherStored','weatherHeadersStored'])assert.doesNotMatch(p.get(id).textContent,/ยังไม่ตั้ง/);
 await p.submit();assert.equal(p.requests.length,0);
 p.get('codexUsageMode').value='both';await p.displaySubmit();assert.deepEqual(p.displayRequests,[{codexUsageMode:'both'}]);
});

test('incomplete current settings never become a writable or unset snapshot',{timeout:30000},async()=>{
 const p=await portal({settings:{autoRotateSeconds:undefined}});
 assert.equal(p.get('settingsButton').disabled,true);
 assert.match(p.get('ssidStored').textContent,/ไม่ทราบ/);
 await p.submit();assert.equal(p.requests.length,0);
});

test('successful commit with failed reload reports saved and blocks stale resubmission',{timeout:30000},async()=>{
 const p=await portal({reloadLoad:false});p.get('weatherLabel').value='Forecast';
 await p.submit();assert.deepEqual(p.requests,[{weather:{label:'Forecast'}}]);
 assert.match(p.get('status').textContent,/บันทึกแล้ว.*โหลด.*ไม่สำเร็จ/);
 assert.ok(p.get('settingsButton'),'missing guarded settings Save');assert.equal(p.get('settingsButton').disabled,true);
 assert.match(p.get('weatherStored').textContent,/ไม่ทราบ/);
 await p.submit();assert.equal(p.requests.length,1);
});

test('edited URL keeps its complete query and replacements are sent only when entered',{timeout:30000},async()=>{
  const p=await portal();
  p.get('stocksUrl').value='https://script.test/exec?accessToken=new-secret';
  p.get('stocksHeaders').value='{"X-Key":"new-header"}';
  p.get('ssid').value='new-network';p.get('wifiPassword').value='new-password';p.get('usageToken').value='0123456789abcdef';
  const patch=await p.submit();
  assert.equal(patch.stocks.url,'https://script.test/exec?accessToken=new-secret');
  assert.deepEqual(patch.stocks.headers,{'X-Key':'new-header'});
  assert.equal(patch.ssid,'new-network');assert.equal(patch.wifiPassword,'new-password');assert.equal(patch.usageToken,'0123456789abcdef');
  assert.equal(p.get('stocksUrl').value,'');assert.equal(p.get('wifiPassword').value,'');assert.equal(p.get('usageToken').value,'');
});

test('clearing sources, Wi-Fi and headers requires explicit actions',{timeout:30000},async()=>{
  const p=await portal();
  p.get('weatherClear').checked=true;p.get('stocksHeaders').value='{}';p.get('clearWifi').checked=true;p.get('clearSsid').checked=true;
  const patch=await p.submit();
  assert.equal(patch.weather.url,'');assert.deepEqual(patch.stocks.headers,{});
  assert.equal(patch.wifiPassword,'');assert.equal(patch.ssid,'');
  assert.equal(Object.hasOwn(patch.stocks,'url'),false);
});

test('malformed edited headers do not send a partial settings update',{timeout:30000},async()=>{
  const p=await portal();p.get('weatherHeaders').value='{"broken"';
  await p.submit();assert.equal(p.requests.length,0);
});


test('firmware selection is manual and oversize/merged-name inputs are rejected',{timeout:30000},async()=>{
 const p=await portal({language:'en'});assert.equal(p.uploads.length,0);
 p.get('firmwareFile').files=[{name:'desk-duo-cyd.bin',size:1900000}];await p.firmwareSubmit();assert.equal(p.uploads.length,0);
 p.get('firmwareFile').files=[{name:'desk-duo-app.bin',size:2000000}];await p.firmwareSubmit();assert.equal(p.uploads.length,0);
});
test('binary upload sets explicit headers; network 100 percent still waits for validation',{timeout:30000},async()=>{
 const p=await portal({language:'en'});const file={name:'desk-duo-app.bin',size:1500000};p.get('firmwareFile').files=[file];await p.firmwareSubmit();
 const req=p.uploads[0];assert.equal(req.method,'POST');assert.equal(req.url,'/api/firmware');assert.equal(req.file,file);
 assert.deepEqual(req.headers,{'Content-Type':'application/octet-stream','X-DeskDuo-Update':'1'});
 req.upload.onprogress({lengthComputable:true,loaded:1500000,total:1500000});
 assert.equal(p.get('firmwareProgress').value,100);assert.match(p.get('firmwareStatus').textContent,/Verifying/i);
 req.status=400;await req.onload();assert.match(p.get('firmwareStatus').textContent,/reject|retry/i);assert.equal(p.get('firmwareButton').disabled,false);
});
test('firmware network interruption enables retry and does not claim success',{timeout:30000},async()=>{
 const p=await portal({language:'en'});p.get('firmwareFile').files=[{name:'desk-duo-app.bin',size:1000}];await p.firmwareSubmit();
 p.uploads[0].onerror();assert.match(p.get('firmwareStatus').textContent,/interrupt|retry/i);assert.equal(p.get('firmwareButton').disabled,false);
});
test('server busy prevents a second upload and explains settings busy response',{timeout:30000},async()=>{
 const p=await portal({language:'en'});p.setFirmware({version:'2.0.0',capacity:1966080,busy:true,runningPartition:'ota_0'});
 p.get('firmwareFile').files=[{name:'desk-duo-app.bin',size:1000}];await p.firmwareSubmit();assert.equal(p.uploads.length,0);assert.match(p.get('firmwareStatus').textContent,/busy/i);
});


test('reconnect confirms a changed running slot, including same-version updates',{timeout:30000},async()=>{
 const p=await portal({language:'en'});p.get('firmwareFile').files=[{name:'desk-duo-app.bin',size:1000}];await p.firmwareSubmit();
 const req=p.uploads[0];req.status=200;const done=req.onload();
 assert.match(p.get('firmwareStatus').textContent,/Restarting/);
 p.setFirmware({version:'2.0.0',capacity:1966080,busy:false,runningPartition:'ota_1'});
 const [id,fn]=p.timers.entries().next().value;p.timers.delete(id);fn();await done;
 assert.match(p.get('firmwareStatus').textContent,/Update complete/);assert.equal(p.get('firmwareButton').disabled,false);
});
test('unchanged slot never reports successful installation and reconnect retries are bounded',{timeout:30000},async()=>{
 const p=await portal({language:'en'});p.get('firmwareFile').files=[{name:'desk-duo-app.bin',size:1000}];await p.firmwareSubmit();
 const req=p.uploads[0];req.status=200;const done=req.onload();
 for(let i=0;i<30;i++){
  assert.equal(p.timers.size,1);const [id,fn]=p.timers.entries().next().value;p.timers.delete(id);fn();await new Promise(resolve=>setImmediate(resolve));
 }
 await done;assert.equal(p.timers.size,0);assert.match(p.get('firmwareStatus').textContent,/could not be confirmed/);assert.equal(p.get('firmwareButton').disabled,false);
});

// Catches display preferences leaking into the secret-bearing settings patch,
// or saving a default before the active preference has loaded.
test('display form loads and saves independently of Wi-Fi/source settings',{timeout:30000},async()=>{
 const p=await portal();assert.equal(p.get('codexUsageMode').value,'weekly');
 p.get('codexUsageMode').value='both';await p.displaySubmit();
 assert.deepEqual(p.displayRequests,[{codexUsageMode:'both'}]);assert.equal(p.requests.length,0);
 await p.submit();assert.equal(p.requests.length,0);
});
test('display save stays guarded when initial load fails',{timeout:30000},async()=>{
 const p=await portal(false);assert.equal(p.get('displayButton').disabled,true);
 await p.displaySubmit();assert.equal(p.displayRequests.length,0);
});

// Catch diagnostics failure disabling saves, writing config, leaking raw errors, or scheduling polling.
test('weather status is read only and failure is isolated from settings and display',{timeout:30000},async()=>{
 const p=await portal({networkLoad:false});
 assert.ok(p.get('networkRefresh'),'missing manual status refresh');
 assert.match(p.get('weatherDataStatus').textContent,/ไม่ทราบ.*ไม่สำเร็จ/);
 assert.doesNotMatch(p.get('weatherDataStatus').textContent,/private/);
 assert.equal(p.get('settingsButton').disabled,false);
 p.get('weatherLabel').value='Forecast';await p.submit();assert.deepEqual(p.requests,[{weather:{label:'Forecast'}}]);
 await p.networkRefresh();assert.ok(p.networkRequests.every(o=>!o.method));assert.equal(p.timers.size,0);
 p.get('codexUsageMode').value='both';await p.displaySubmit();assert.equal(p.displayRequests.length,1);
});
test('manual status reports actual stale values and old generation waits for new settings',{timeout:30000},async()=>{
 const p=await portal();assert.ok(p.get('networkRefresh'),'missing manual status refresh');
 assert.match(p.get('weatherDataStatus').textContent,/ยังไม่มีข้อมูล.*รอ/);
 const value={stationOnline:true,weatherConfigured:true,currentGeneration:8,weather:{hasData:true,unknown:false,stale:true,ageMs:61000,temperature:29.6,humidity:76,weatherCode:3,hourlyCount:4},attempt:{attempted:true,currentGeneration:true,outcome:'http_status',httpStatus:503,tlsCode:-12345}};
 p.setNetwork(value);await p.networkRefresh();assert.match(p.get('weatherDataStatus').textContent,/เก่า.*29.6.*76.*503/);assert.doesNotMatch(p.get('weatherDataStatus').textContent,/12345/);
 value.weather.hasData=false;value.attempt.outcome='accepted';value.attempt.currentGeneration=false;p.setNetwork(value);await p.networkRefresh();assert.match(p.get('weatherDataStatus').textContent,/รอ.*ตั้งค่าปัจจุบัน/);assert.doesNotMatch(p.get('weatherDataStatus').textContent,/รับข้อมูลแล้ว/);
 assert.equal(p.requests.length,0);assert.equal(p.timers.size,0);
});
test('weather failures use friendly clock HTTPS HTTP and format statuses',{timeout:30000},async()=>{
 const p=await portal();assert.ok(p.get('networkRefresh'),'missing manual status refresh');
 for(const [outcome,text] of [['clock_wait',/รอเวลา/],['tls_failure',/HTTPS/],['dns_failure',/เครือข่าย/],['parse_failure',/รูปแบบ/],['http_status',/HTTP 429/]]){
 p.setNetwork({stationOnline:true,weatherConfigured:true,weather:{hasData:false},attempt:{attempted:true,currentGeneration:true,outcome,httpStatus:429}});await p.networkRefresh();assert.match(p.get('weatherDataStatus').textContent,text);
 }
});

// Catch acceptance from the pre-save source remaining visible after a source change.
test('committed settings invalidate old acceptance including an in-flight status response',{timeout:30000},async()=>{
 const accepted={stationOnline:true,weatherConfigured:true,weather:{hasData:true,temperature:29.6},attempt:{attempted:true,currentGeneration:true,outcome:'accepted'}};
 const p=await portal({network:accepted});assert.match(p.get('weatherDataStatus').textContent,/รับข้อมูลแล้ว/);
 p.get('weatherLabel').value='New source';await p.submit();assert.doesNotMatch(p.get('weatherDataStatus').textContent,/รับข้อมูลแล้ว/);assert.match(p.get('weatherDataStatus').textContent,/รอ.*สถานะ.*ปัจจุบัน/);
 let finish;const delayed=await portal({networkReply:()=>new Promise(resolve=>{finish=resolve;})});
 delayed.get('weatherLabel').value='New source';await delayed.submit();finish({ok:true,json:async()=>accepted});
 await new Promise(resolve=>setImmediate(resolve));
 assert.doesNotMatch(delayed.get('weatherDataStatus').textContent,/รับข้อมูลแล้ว/);assert.equal(delayed.get('networkRefresh').disabled,false);
});

// Fails if the saved display language never reaches the actual rendered form.
test('saved language governs portal and selection previews without claiming a save',{timeout:30000},async()=>{
 const p=await portal({language:'en'});
 assert.ok(p.get('language'),'language selector must exist');
 assert.equal(p.get('language').value,'en');
 assert.equal(p.document.documentElement.lang,'en');assert.equal(p.document.title,'Desk Duo setup');
 assert.ok(p.translationNodes.some(n=>n.textContent==='Save display settings'));
 assert.match(p.get('weatherDataStatus').textContent,/Weather data:.*No data/);
 p.get('weatherLabel').value='ต้นฉบับ';p.get('language').value='th';p.get('language').change();
 assert.equal(p.document.documentElement.lang,'th');assert.match(p.get('languageStored').textContent,/อังกฤษ.*ยังไม่ได้บันทึก/);
 assert.match(p.get('weatherDataStatus').textContent,/ข้อมูลอากาศ:/);
 assert.equal(p.get('weatherLabel').value,'ต้นฉบับ');assert.equal(p.displayRequests.length,0);
 await p.displaySubmit();assert.deepEqual(p.displayRequests,[{language:'th'}]);
 assert.equal(p.get('codexUsageMode').value,'weekly');assert.match(p.get('languageStored').textContent,/ไทย/);
 assert.doesNotMatch(p.get('languageStored').textContent,/ยังไม่ได้บันทึก/);
 p.get('codexUsageMode').value='both';await p.displaySubmit();
 assert.deepEqual(p.displayRequests.at(-1),{codexUsageMode:'both'});
 assert.equal(p.get('language').value,'th');
});

test('failed display save and failed readback never claim selected language saved',{timeout:30000},async()=>{
 const p=await portal({displaySave:false});p.get('language').value='en';p.get('language').change();
 await p.displaySubmit();assert.match(p.get('displayStatus').textContent,/Could not save/);
 assert.match(p.get('languageStored').textContent,/Thai.*Not saved/);assert.equal(p.get('codexUsageMode').value,'weekly');
 const q=await portal({displayReload:false});q.get('weatherLabel').value='Unsaved editor';q.get('language').value='en';q.get('language').change();q.get('codexUsageMode').value='both';
 await q.displaySubmit();assert.deepEqual(q.displayRequests,[{codexUsageMode:'both',language:'en'}]);
 assert.match(q.get('displayStatus').textContent,/Saved.*readback failed/);assert.equal(q.get('displayButton').disabled,true);
 assert.equal(q.get('languageStored').textContent,'Saved language unknown');
 assert.equal(q.get('language').value,'en');assert.equal(q.get('codexUsageMode').value,'both');
 assert.equal(q.get('weatherLabel').value,'Unsaved editor');assert.equal(q.requests.length,0);
 q.get('language').value='th';q.get('language').change();assert.match(q.get('languageStored').textContent,/ไม่ทราบ/);
 assert.doesNotMatch(q.get('languageStored').textContent,/ยังไม่ได้บันทึก/);
 await q.displaySubmit();assert.equal(q.displayRequests.length,1);
});

test('firmware picker copy follows selected language and keeps exact selected filename',{timeout:30000},async()=>{
 const p=await portal();assert.ok(p.get('firmwareSelected'),'selected file copy must be localized');
 assert.match(p.get('firmwareSelected').textContent,/ยังไม่เลือก/);
 p.get('firmwareFile').files=[{name:'desk-duo-app.bin'}];p.get('firmwareFile').change();
 assert.equal(p.get('firmwareSelected').textContent,'desk-duo-app.bin');
 p.get('language').value='en';p.get('language').change();assert.equal(p.get('firmwareSelected').textContent,'desk-duo-app.bin');
 p.get('firmwareFile').files=[];p.get('firmwareFile').change();assert.equal(p.get('firmwareSelected').textContent,'No file selected');
});

// Exercises production script state and actual sparse request bodies.
test('brightness previews numbers, sparse/combined saves and locale preserve editors',{timeout:30000},async()=>{
 const p=await portal({language:'en'});
 assert.equal(p.get('brightnessPercent').value,100);
 assert.match(p.get('brightnessStored').textContent,/100%/);
 p.get('weatherLabel').value='Keep user label';
 p.get('brightnessPercent').value='5';p.get('brightnessPercent').input();
 assert.equal(p.get('brightnessValue').textContent,'5%');
 assert.match(p.get('brightnessStored').textContent,/100%.*Not saved/);
 p.get('language').value='th';p.get('language').change();
 assert.equal(p.get('brightnessPercent').value,'5');assert.equal(p.get('weatherLabel').value,'Keep user label');
 await p.displaySubmit();assert.deepEqual(p.displayRequests,[{language:'th',brightnessPercent:5}]);
 p.get('brightnessPercent').value='100';p.get('brightnessPercent').input();await p.displaySubmit();
 assert.deepEqual(p.displayRequests[1],{brightnessPercent:100});
 assert.equal(p.get('weatherLabel').value,'Keep user label');
});
test('brightness failure retains saved value and accepted readback failure invalidates it',{timeout:30000},async()=>{
 const p=await portal({language:'en',displaySave:false});p.get('brightnessPercent').value='50';p.get('brightnessPercent').input();await p.displaySubmit();
 assert.match(p.get('brightnessStored').textContent,/100%.*Not saved/);assert.equal(p.get('brightnessPercent').value,'50');
 const q=await portal({language:'en',displayReload:false});q.get('brightnessPercent').value='5';q.get('brightnessPercent').input();await q.displaySubmit();
 assert.equal(q.get('brightnessStored').textContent,'Saved brightness unknown');assert.equal(q.get('displayButton').disabled,true);assert.equal(q.get('brightnessPercent').value,'5');
});
test('invalid or missing brightness readback never establishes saved display state',{timeout:30000},async()=>{
 for(const value of [undefined,null,true,4,101,50.5,'50']){
  const p=await portal({language:'en',display:{brightnessPercent:value}});
  assert.equal(p.get('displayButton').disabled,true);assert.match(p.get('brightnessStored').textContent,/ไม่ทราบ|unknown/);
 }
 for(const value of [5,100]){const p=await portal({display:{brightnessPercent:value}});assert.equal(p.get('displayButton').disabled,false);}
});

test('Auto schedule controls save sparse fields and display saved versus applied',{timeout:30000},async()=>{
 const p=await portal({language:'en'});
 assert.ok(p.get('brightnessAuto'),'Auto control must exist');
 assert.equal(p.get('brightnessDayStart').value,'06:00');assert.equal(p.get('brightnessNightStart').value,'18:00');
 p.get('brightnessAuto').checked=true;p.get('brightnessDayStart').value='07:15';p.get('brightnessNightStart').value='19:45';await p.displaySubmit();
 assert.deepEqual(p.displayRequests,[{brightnessAuto:true,brightnessDayStart:'07:15',brightnessNightStart:'19:45'}]);
 assert.match(p.get('brightnessScheduleStored').textContent,/07:15.*19:45/);assert.match(p.get('brightnessEffective').textContent,/100%.*Unknown/);
});

test('read-only 15s refresh preserves dirty base/times/Auto/locale and user labels; clean editors follow saved',{timeout:30000},async()=>{
 const p=await portal({language:'en'});assert.equal(p.displayTimers.size,1);
 p.get('brightnessPercent').value='11';p.get('brightnessPercent').input();p.get('brightnessAuto').checked=true;p.get('brightnessDayStart').value='08:15';p.get('brightnessNightStart').value='20:30';p.get('language').value='th';p.get('language').change();p.get('weatherLabel').value='User label';
 p.setDisplay({...p.displaySnapshot(),codexUsageMode:'both',brightnessPercent:75,brightnessAuto:true,brightnessDayStart:'07:45',brightnessNightStart:'19:45',brightnessTargetPercent:38,brightnessEffectivePercent:38,brightnessPhase:'night'});
 await p.pollDisplay();assert.equal(p.displayRequests.length,0);assert.equal(p.get('codexUsageMode').value,'both');assert.equal(p.get('brightnessPercent').value,'11');assert.equal(p.get('brightnessDayStart').value,'08:15');assert.equal(p.get('brightnessNightStart').value,'20:30');assert.equal(p.get('language').value,'th');assert.equal(p.document.documentElement.lang,'th');assert.equal(p.get('weatherLabel').value,'User label');assert.match(p.get('brightnessStored').textContent,/75%/);assert.match(p.get('brightnessEffective').textContent,/38%/);
 await p.displaySubmit();assert.deepEqual(p.displayRequests,[{language:'th',brightnessPercent:11,brightnessDayStart:'08:15',brightnessNightStart:'20:30'}]);
});
test('invalid/equal schedules block POST; invalid six-field readback never claims known; null applied remains valid',{timeout:30000},async()=>{
 for(const field of ['brightnessDayStart','brightnessNightStart'])for(const value of ['',null,'6:00','24:00','12:60',true]){const p=await portal({display:{[field]:value}});assert.equal(p.get('displayButton').disabled,true);}
 for(const value of [null,1,'true',undefined]){const p=await portal({display:{brightnessAuto:value}});assert.equal(p.get('displayButton').disabled,true);}
 const p=await portal({language:'en'});for(const value of ['18:00','24:00','6:00']){p.get('brightnessDayStart').value=value;await p.displaySubmit();assert.equal(p.displayRequests.length,0);assert.match(p.get('displayStatus').textContent,/HH:MM/);}
 const q=await portal({language:'en',display:{brightnessEffectivePercent:null}});assert.equal(q.get('displayButton').disabled,false);assert.match(q.get('brightnessEffective').textContent,/Last applied Unknown/);
});
test('stale pre-POST GET loses authority; accepted missing/bad readback stays unknown until recovery',{timeout:30000},async()=>{
 const p=await portal({language:'en'});let finish;
 const stale=p.displaySnapshot();p.scenario.displayReply=()=>new Promise(resolve=>finish=resolve);
 const polling=p.pollDisplay();await new Promise(resolve=>setImmediate(resolve));
 p.get('brightnessPercent').value='11';const saving=p.displaySubmit();await new Promise(resolve=>setImmediate(resolve));
 assert.equal(p.get('brightnessStored').textContent,'Saved brightness unknown');
 p.scenario.displayReply=async()=>({ok:true,json:async()=>({...stale,brightnessAuto:undefined})});
 finish({ok:true,json:async()=>stale});await polling;await saving;
 assert.equal(p.get('brightnessStored').textContent,'Saved brightness unknown');assert.equal(p.get('displayButton').disabled,true);assert.equal(p.get('brightnessPercent').value,'11');
 delete p.scenario.displayReply;await p.pollDisplay();assert.match(p.get('brightnessStored').textContent,/11%/);assert.equal(p.get('displayButton').disabled,false);
});
test('failed POST retains snapshot while old GET cannot publish and edits during GET survive',{timeout:30000},async()=>{
 const p=await portal({language:'en',displaySave:false});let finish;p.scenario.displayReply=()=>new Promise(resolve=>finish=resolve);
 const polling=p.pollDisplay();await new Promise(resolve=>setImmediate(resolve));p.get('brightnessPercent').value='11';p.get('brightnessPercent').input();await p.displaySubmit();
 finish({ok:true,json:async()=>({...p.displaySnapshot(),brightnessPercent:75})});await polling;assert.match(p.get('brightnessStored').textContent,/100%.*Not saved/);assert.equal(p.get('brightnessPercent').value,'11');assert.match(p.get('displayStatus').textContent,/Could not save/);
 delete p.scenario.displayReply;await p.pollDisplay();assert.equal(p.get('brightnessPercent').value,'11');
});

test('status read failure stays unknown while editing or changing locale, then valid GET recovers',{timeout:30000},async()=>{
 const p=await portal({language:'en',display:{brightnessEffectivePercent:50}});
 p.scenario.displayReply=async()=>{throw Error('offline');};await p.pollDisplay();assert.equal(p.get('brightnessEffective').textContent,'Brightness status: Unknown');
 p.get('brightnessPercent').value='11';p.get('brightnessPercent').input();assert.equal(p.get('brightnessEffective').textContent,'Brightness status: Unknown');
 p.get('language').value='th';p.get('language').change();assert.match(p.get('brightnessEffective').textContent,/ไม่ทราบ/);assert.doesNotMatch(p.get('brightnessEffective').textContent,/50%/);
 delete p.scenario.displayReply;await p.pollDisplay();assert.match(p.get('brightnessEffective').textContent,/50%/);assert.equal(p.get('brightnessPercent').value,'11');
});

test('15s refresh permits only one outstanding GET and skips a hidden page without writes',{timeout:30000},async()=>{
 const p=await portal();let calls=0,finish;p.scenario.displayReply=()=>{calls++;return new Promise(resolve=>finish=resolve);};
 const request=p.refreshDisplay();await new Promise(resolve=>setImmediate(resolve));await p.refreshDisplay();assert.equal(calls,1);assert.equal(p.displayRequests.length,0);
 finish({ok:true,json:async()=>p.displaySnapshot()});await request;
 p.document.hidden=true;await p.pollDisplay();assert.equal(calls,1);assert.equal(p.displayTimers.size,1);
 p.document.hidden=false;delete p.scenario.displayReply;await p.pollDisplay();assert.equal(p.displayRequests.length,0);
});

test('unknown hardware fields do not invalidate saved six fields or render object properties as phases',{timeout:30000},async()=>{
 for(const phase of [undefined,null,'unexpected','__proto__','constructor']){
  const p=await portal({language:'en',display:{brightnessTargetPercent:null,brightnessEffectivePercent:null,brightnessPhase:phase}});
  assert.equal(p.get('displayButton').disabled,false);assert.match(p.get('brightnessStored').textContent,/100%/);assert.match(p.get('brightnessEffective').textContent,/Unknown$/);
 }
});

test('accepted POST rebases untouched concurrent fields, keeps later edits, and next saves stay sparse',{timeout:30000},async()=>{
 const p=await portal({language:'en'});
 p.setDisplay({...p.displaySnapshot(),brightnessDayStart:'07:15'});
 p.get('brightnessPercent').value='50';p.get('brightnessPercent').input();await p.displaySubmit();
 assert.deepEqual(p.displayRequests,[{brightnessPercent:50}]);assert.match(p.get('brightnessScheduleStored').textContent,/07:15/);assert.equal(p.get('brightnessDayStart').value,'07:15');assert.doesNotMatch(p.get('brightnessScheduleStored').textContent,/Not saved/);
 p.get('brightnessPercent').value='60';p.get('brightnessPercent').input();await p.displaySubmit();assert.deepEqual(p.displayRequests[1],{brightnessPercent:60});assert.equal(p.displaySnapshot().brightnessDayStart,'07:15');
 let finish;p.scenario.displayReply=()=>new Promise(resolve=>finish=resolve);
 p.setDisplay({...p.displaySnapshot(),brightnessDayStart:'08:30',brightnessNightStart:'20:00',codexUsageMode:'both'});
 p.get('brightnessPercent').value='70';p.get('brightnessPercent').input();const saving=p.displaySubmit();await new Promise(resolve=>setImmediate(resolve));
 assert.equal(p.get('brightnessStored').textContent,'Saved brightness unknown');assert.equal(p.get('displayButton').disabled,true);
 p.get('brightnessPercent').value='80';p.get('brightnessPercent').input();p.get('brightnessNightStart').value='21:45';p.get('brightnessNightStart').change();p.get('language').value='th';p.get('language').change();p.get('weatherLabel').value='Keep user data';
 finish({ok:true,json:async()=>p.displaySnapshot()});await saving;
 assert.equal(p.get('brightnessDayStart').value,'08:30');assert.equal(p.get('codexUsageMode').value,'both');assert.equal(p.get('brightnessPercent').value,'80');assert.equal(p.get('brightnessNightStart').value,'21:45');assert.equal(p.get('language').value,'th');assert.equal(p.document.documentElement.lang,'th');assert.equal(p.get('weatherLabel').value,'Keep user data');assert.match(p.get('brightnessStored').textContent,/70%/);
 delete p.scenario.displayReply;await p.displaySubmit();assert.deepEqual(p.displayRequests[2],{brightnessPercent:70});assert.deepEqual(p.displayRequests[3],{language:'th',brightnessPercent:80,brightnessNightStart:'21:45'});assert.equal(p.displaySnapshot().brightnessDayStart,'08:30');
});

test('first valid recovery after failed or invalid initial GET preserves edited fields and rebases untouched ones',{timeout:30000},async()=>{
 for(const initial of [{displayLoad:false},{display:{brightnessAuto:undefined}}]){
  const p=await portal(initial);assert.equal(p.get('displayButton').disabled,true);
  p.get('language').value='en';p.get('language').change();p.get('brightnessPercent').value='11';p.get('brightnessPercent').input();p.get('brightnessDayStart').value='07:15';p.get('brightnessDayStart').change();p.get('weatherLabel').value='Keep user data';
  assert.equal(p.get('brightnessStored').textContent,'Saved brightness unknown');await p.displaySubmit();assert.equal(p.displayRequests.length,0);
  p.setDisplay({...p.displaySnapshot(),brightnessAuto:true,brightnessNightStart:'20:30'});p.scenario.displayReply=async()=>({ok:true,json:async()=>p.displaySnapshot()});await p.pollDisplay();
  assert.equal(p.get('language').value,'en');assert.equal(p.document.documentElement.lang,'en');assert.equal(p.get('brightnessPercent').value,'11');assert.equal(p.get('brightnessDayStart').value,'07:15');assert.equal(p.get('weatherLabel').value,'Keep user data');
  assert.equal(p.get('codexUsageMode').value,'weekly');assert.equal(p.get('brightnessAuto').checked,true);assert.equal(p.get('brightnessNightStart').value,'20:30');assert.equal(p.get('displayButton').disabled,false);assert.match(p.get('brightnessStored').textContent,/100%.*Not saved/);
  await p.displaySubmit();assert.deepEqual(p.displayRequests,[{language:'en',brightnessPercent:11,brightnessDayStart:'07:15'}]);
 }
});
test('edits during the first pending GET survive valid readback and the next save stays sparse',{timeout:30000},async()=>{
 let finish;const p=await portal({display:{brightnessAuto:true,brightnessNightStart:'20:30'},displayReply:()=>new Promise(resolve=>finish=resolve)});
 assert.equal(p.get('displayButton').disabled,true);p.get('language').value='en';p.get('language').change();p.get('brightnessPercent').value='11';p.get('brightnessPercent').input();p.get('brightnessDayStart').value='07:15';p.get('brightnessDayStart').change();
 finish({ok:true,json:async()=>p.displaySnapshot()});await new Promise(resolve=>setImmediate(resolve));
 assert.equal(p.get('language').value,'en');assert.equal(p.document.documentElement.lang,'en');assert.equal(p.get('brightnessPercent').value,'11');assert.equal(p.get('brightnessDayStart').value,'07:15');assert.equal(p.get('brightnessNightStart').value,'20:30');assert.equal(p.get('brightnessAuto').checked,true);assert.equal(p.get('codexUsageMode').value,'weekly');assert.equal(p.get('displayButton').disabled,false);assert.equal(p.displayRequests.length,0);
 delete p.scenario.displayReply;await p.displaySubmit();assert.deepEqual(p.displayRequests,[{language:'en',brightnessPercent:11,brightnessDayStart:'07:15'}]);
});
