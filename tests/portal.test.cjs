const {test} = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const production = fs.readFileSync(path.join(__dirname, '../firmware/main/assets.cpp'), 'utf8').split('<script>')[1].split('</script>')[0];

async function portal() {
  const elements = new Map();
  const get = id => {
    if (!elements.has(id)) elements.set(id, {value:'', checked:false, textContent:'', append(){}, addEventListener(name, handler){this[name]=handler;}});
    return elements.get(id);
  };
  const settings = {autoRotateSeconds:20, touch:{swapAxes:false,invertX:false,invertY:false,minX:0,maxX:4095,minY:0,maxY:4095},
    weather:{url:'https://weather.test/data',label:'Bangkok',fields:{temperature:'current.temperature_2m'},refreshSeconds:900},
    stocks:{url:'https://script.test/exec',refreshSeconds:300}};
  const requests = [];
  const context = vm.createContext({document:{getElementById:get,createElement(){return {innerHTML:''};}},
    fetch:async (url, options) => {
      if(options.method)requests.push(JSON.parse(options.body));
      return {ok:true,status:options.method?204:200,json:async()=>structuredClone(settings)};
    }});
  vm.runInContext(production,context);
  await new Promise(resolve=>setImmediate(resolve));
  const submit = async () => {await get('settings').submit({preventDefault(){}}); return requests.at(-1);};
  return {get,submit,requests};
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
