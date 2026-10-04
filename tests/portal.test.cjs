const {test} = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const production = fs.readFileSync(path.join(__dirname, '../firmware/main/assets.cpp'), 'utf8').split('<script>')[1].split('</script>')[0];

async function portal(displayLoad=true) {
  const uploads=[];let firmware={version:"2.0.0",capacity:1966080,busy:false,runningPartition:"ota_0"};
  const timers=new Map();let nextTimer=0;
  const elements = new Map();
  const get = id => {
    if (!elements.has(id)) elements.set(id, {value:'', checked:false, textContent:'', append(){}, addEventListener(name, handler){this[name]=handler;}});
    return elements.get(id);
  };
  const settings = {autoRotateSeconds:20, touch:{swapAxes:false,invertX:false,invertY:false,minX:0,maxX:4095,minY:0,maxY:4095},
    weather:{url:'https://weather.test/data',label:'Bangkok',fields:{temperature:'current.temperature_2m'},refreshSeconds:900},
    stocks:{url:'https://script.test/exec',refreshSeconds:300}};
  const requests = [];const displayRequests=[];
  const context = vm.createContext({
    setTimeout(fn){const id=++nextTimer;timers.set(id,fn);return id;},clearTimeout(id){timers.delete(id);},AbortController,
    XMLHttpRequest:class {constructor(){this.upload={};this.headers={};uploads.push(this);}open(method,url){this.method=method;this.url=url;}setRequestHeader(k,v){this.headers[k]=v;}send(file){this.file=file;}},
    document:{getElementById:get,createElement(){return {innerHTML:''};}},
    fetch:async (url, options) => {
      if(url==='/api/display'){if(!options.method){if(!displayLoad)throw Error('offline');return {ok:true,json:async()=>({codexUsageMode:'weekly'})};}displayRequests.push(JSON.parse(options.body));return {ok:true,status:204};}
      if(url==='/api/firmware')return {ok:true,status:200,json:async()=>structuredClone(firmware)};
      if(options.method)requests.push(JSON.parse(options.body));
      return {ok:true,status:options.method?204:200,json:async()=>structuredClone(settings)};
    }});
  vm.runInContext(production,context);
  await new Promise(resolve=>setImmediate(resolve));
  const submit = async () => {await get('settings').submit({preventDefault(){}}); return requests.at(-1);};
  return {get,submit,requests,displayRequests,displaySubmit:()=>get("display").submit({preventDefault(){}}),uploads,timers,setFirmware(value){firmware=value;},firmwareSubmit:()=>get("firmware").submit({preventDefault(){}})};
}

test('unchanged settings omit redacted URL queries and all hidden credentials', async()=>{
  const p=await portal();
  const patch=await p.submit();
  for(const key of ['ssid','wifiPassword','usageToken','setupPassword']) assert.equal(Object.hasOwn(patch,key),false,key);
  for(const source of ['weather','stocks']) {
    assert.equal(Object.hasOwn(patch[source],'url'),false,source+' URL');
    assert.equal(Object.hasOwn(patch[source],'headers'),false,source+' headers');
  }
  assert.equal(patch.weather.refreshSeconds,900);
});

test('edited URL keeps its complete query and replacements are sent only when entered', async()=>{
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

test('clearing sources, Wi-Fi and headers requires explicit actions', async()=>{
  const p=await portal();
  p.get('weatherClear').checked=true;p.get('stocksHeaders').value='{}';p.get('clearWifi').checked=true;p.get('clearSsid').checked=true;
  const patch=await p.submit();
  assert.equal(patch.weather.url,'');assert.deepEqual(patch.stocks.headers,{});
  assert.equal(patch.wifiPassword,'');assert.equal(patch.ssid,'');
  assert.equal(Object.hasOwn(patch.stocks,'url'),false);
});

test('malformed edited headers do not send a partial settings update',async()=>{
  const p=await portal();p.get('weatherHeaders').value='{"broken"';
  await p.submit();assert.equal(p.requests.length,0);
});


test('firmware selection is manual and oversize/merged-name inputs are rejected',async()=>{
 const p=await portal();assert.equal(p.uploads.length,0);
 p.get('firmwareFile').files=[{name:'desk-duo-cyd.bin',size:1900000}];await p.firmwareSubmit();assert.equal(p.uploads.length,0);
 p.get('firmwareFile').files=[{name:'desk-duo-app.bin',size:2000000}];await p.firmwareSubmit();assert.equal(p.uploads.length,0);
});
test('binary upload sets explicit headers; network 100 percent still waits for validation',async()=>{
 const p=await portal();const file={name:'desk-duo-app.bin',size:1500000};p.get('firmwareFile').files=[file];await p.firmwareSubmit();
 const req=p.uploads[0];assert.equal(req.method,'POST');assert.equal(req.url,'/api/firmware');assert.equal(req.file,file);
 assert.deepEqual(req.headers,{'Content-Type':'application/octet-stream','X-DeskDuo-Update':'1'});
 req.upload.onprogress({lengthComputable:true,loaded:1500000,total:1500000});
 assert.equal(p.get('firmwareProgress').value,100);assert.match(p.get('firmwareStatus').textContent,/Verifying/i);
 req.status=400;await req.onload();assert.match(p.get('firmwareStatus').textContent,/reject|retry/i);assert.equal(p.get('firmwareButton').disabled,false);
});
test('firmware network interruption enables retry and does not claim success',async()=>{
 const p=await portal();p.get('firmwareFile').files=[{name:'desk-duo-app.bin',size:1000}];await p.firmwareSubmit();
 p.uploads[0].onerror();assert.match(p.get('firmwareStatus').textContent,/interrupt|retry/i);assert.equal(p.get('firmwareButton').disabled,false);
});
test('server busy prevents a second upload and explains settings busy response',async()=>{
 const p=await portal();p.setFirmware({version:'2.0.0',capacity:1966080,busy:true,runningPartition:'ota_0'});
 p.get('firmwareFile').files=[{name:'desk-duo-app.bin',size:1000}];await p.firmwareSubmit();assert.equal(p.uploads.length,0);assert.match(p.get('firmwareStatus').textContent,/busy/i);
});


test('reconnect confirms a changed running slot, including same-version updates',async()=>{
 const p=await portal();p.get('firmwareFile').files=[{name:'desk-duo-app.bin',size:1000}];await p.firmwareSubmit();
 const req=p.uploads[0];req.status=200;const done=req.onload();
 assert.match(p.get('firmwareStatus').textContent,/Restarting/);
 p.setFirmware({version:'2.0.0',capacity:1966080,busy:false,runningPartition:'ota_1'});
 const [id,fn]=p.timers.entries().next().value;p.timers.delete(id);fn();await done;
 assert.match(p.get('firmwareStatus').textContent,/Update complete/);assert.equal(p.get('firmwareButton').disabled,false);
});
test('unchanged slot never reports successful installation and reconnect retries are bounded',async()=>{
 const p=await portal();p.get('firmwareFile').files=[{name:'desk-duo-app.bin',size:1000}];await p.firmwareSubmit();
 const req=p.uploads[0];req.status=200;const done=req.onload();
 for(let i=0;i<30;i++){
  assert.equal(p.timers.size,1);const [id,fn]=p.timers.entries().next().value;p.timers.delete(id);fn();await new Promise(resolve=>setImmediate(resolve));
 }
 await done;assert.equal(p.timers.size,0);assert.match(p.get('firmwareStatus').textContent,/could not be confirmed/);assert.equal(p.get('firmwareButton').disabled,false);
});

// Catches display preferences leaking into the secret-bearing settings patch,
// or saving a default before the active preference has loaded.
test('display form loads and saves independently of Wi-Fi/source settings',async()=>{
 const p=await portal();assert.equal(p.get('codexUsageMode').value,'weekly');
 p.get('codexUsageMode').value='both';await p.displaySubmit();
 assert.deepEqual(p.displayRequests,[{codexUsageMode:'both'}]);assert.equal(p.requests.length,0);
 const patch=await p.submit();assert.equal(Object.hasOwn(patch,'codexUsageMode'),false);
});
test('display save stays guarded when initial load fails',async()=>{
 const p=await portal(false);assert.equal(p.get('displayButton').disabled,true);
 await p.displaySubmit();assert.equal(p.displayRequests.length,0);
});
