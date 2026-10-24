// Exercise the production HTML in a real offline Chromium DOM. Run with the
// existing Playwright runtime via NODE_PATH; no page or dependency is fetched.
// Run scripts/test_firmware.py first: the Weather GET regression uses its
// firmware_core_test helper to obtain the actual production redacted response.
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const {chromium}=require('playwright');
const html=fs.readFileSync(path.join(__dirname,'../firmware/main/assets.cpp'),'utf8').split('R"HTML(')[1].split(')HTML"')[0];
let browser;
test.before(async()=>{fs.mkdirSync(path.join(__dirname,"../.platformio/native-ui-regression"),{recursive:true});browser=await chromium.launch({headless:true,...(process.env.PLAYWRIGHT_CHROMIUM_EXECUTABLE?{executablePath:process.env.PLAYWRIGHT_CHROMIUM_EXECUTABLE}:{})});});
test.after(async()=>{if(browser)await browser.close();});
async function open(options={}){
 const page=await browser.newPage({viewport:{width:options.width||720,height:1280}});
 await page.route('**/*',route=>route.abort());
 await page.evaluate(options=>{
  window.displayPollers=[];const nativeTimeout=window.setTimeout;
  window.setTimeout=(fn,ms,...args)=>ms===15000?(displayPollers.push(fn),-displayPollers.length):nativeTimeout(fn,ms,...args);
  window.scenario=options;window.configPosts=[];window.configSnapshot=options.publicConfig||null;window.privateWeatherUrl=options.privateWeatherUrl;window.posts=[];window.display={codexUsageMode:'weekly',language:options.language||'th',brightnessPercent:100,brightnessAuto:false,brightnessDayStart:'06:00',brightnessNightStart:'18:00',brightnessTargetPercent:100,brightnessEffectivePercent:null,brightnessPhase:'manual',...options.display};window.displayLoads=0;
  window.fetch=async(url,request={})=>{
   if(url==='/api/display'){
    if(request.method){const patch=JSON.parse(request.body);posts.push(patch);if(!scenario.saveFails)Object.assign(display,patch);return {ok:!scenario.saveFails,status:scenario.saveFails?503:204};}
    if(scenario.firstDisplayPending && !displayLoads++){return await new Promise(resolve=>window.releaseInitialDisplay=()=>resolve({ok:true,json:async()=>({...display})}));}
    if(scenario.initialDisplayFails && !displayLoads)throw Error('initial display failure');
    if(++displayLoads>1 && scenario.reloadFails)throw Error('private readback failure');
    return {ok:true,json:async()=>({...display})};
   }
   if(url==='/api/firmware')return {ok:true,json:async()=>({version:'2.0.0',capacity:1966080,busy:false,runningPartition:'ota_0'})};
   if(url==='/api/network')return {ok:true,json:async()=>({stationOnline:true,weatherConfigured:true,epochSeconds:options.epochSeconds,weather:{hasData:true,stale:true,temperature:0,humidity:0,weatherCode:3,hourlyCount:4,ageMs:61000,...options.weather},attempt:{attempted:true,currentGeneration:true,outcome:'tls_failure'}})};
   if(url==='/api/config'){
    if(scenario.configFails)throw Error('private config failure');
    if(request.method){const patch=JSON.parse(request.body);configPosts.push(patch);if(scenario.configSaveFails)return {ok:false,status:503};if(patch.ssid!==undefined)configSnapshot.currentSSID=patch.ssid;if(patch.weather?.url!==undefined)privateWeatherUrl=patch.weather.url;for(const n of ['weather','stocks'])if(patch[n])Object.assign(configSnapshot[n],patch[n]);return {ok:true,status:204};}
    if(configSnapshot){const result=structuredClone(configSnapshot);for(const n of ['weather','stocks']){const source=result[n];source.urlDetailsHidden=source.urlDetailsHidden||/[?#]/.test(source.url);if(n==='weather'&&source.url.includes('?')){const u=new URL(source.url);if(u.origin==='https://api.open-meteo.com'&&u.pathname==='/v1/forecast'){source.latitude=Number(u.searchParams.get('latitude'));source.longitude=Number(u.searchParams.get('longitude'));}}source.url=source.url.split(/[?#]/)[0];}return {ok:true,json:async()=>result};}
    return {ok:true,json:async()=> (configSnapshot={currentSSID:'ต้นฉบับ <data>',wifiPasswordConfigured:true,usageTokenConfigured:true,autoRotateSeconds:20,
     touch:{swapAxes:false,invertX:true,invertY:false,minX:294,maxX:3766,minY:248,maxY:3632},
     weather:{url:'https://weather.test/data',headersConfigured:true,urlDetailsHidden:true,label:'กรุงเทพฯ',fields:{temperature:'current.temperature_2m'},refreshSeconds:900},
     stocks:{url:'https://stocks.test/feed',headersConfigured:false,urlDetailsHidden:false,refreshSeconds:300}})};
   }
   throw Error('Unexpected fetch '+url);
  };
 },options);
 await page.setContent(html);
 if(!options.navigation)await page.evaluate(()=>{activeSection=undefined;for(const section of document.querySelectorAll('.section-content'))section.hidden=false;document.getElementById('displayButton').hidden=false;document.getElementById('settingsButton').hidden=false;delete document.getElementById('displayButton').dataset.scope;delete document.getElementById('settingsButton').dataset.scope;for(const details of document.querySelectorAll('details'))details.open=true;});
 await page.waitForFunction(()=>window.releaseInitialDisplay || document.getElementById('displayStatus').textContent || !document.getElementById('displayButton').disabled);
 return page;
}
test('actual stock DOM saves Yahoo selection, preserves locale drafts and fits narrow viewport',{timeout:30000},async()=>{
 const p=await open({language:'en',width:320});
 await p.locator('#stocksProvider').selectOption('yahoo');
 await p.locator('#stockMarket0').selectOption('TH');await p.locator('#stockSymbol0').fill('PTT');
 await p.locator('#stockEnabled0').check();await p.locator('#stocksRange').selectOption('5d');
 assert.match(await p.locator('#stocksSelectionStored').textContent(),/Not saved/);
 await p.locator('#language').selectOption('th');assert.equal(await p.locator('#stockSymbol0').inputValue(),'PTT');
 await p.evaluate(()=>{
  const previous=fetch;let current;window.stockPosts=[];
  window.fetch=async(url,request={})=>{
   if(url!=='/api/config')return previous(url,request);
   if(request.method){current=await (await previous(url,{})).json();const patch=JSON.parse(request.body);stockPosts.push(patch);Object.assign(current.stocks,{provider:'json',chartRange:'1d',watchlist:[]},patch.stocks);return {ok:true,status:204};}
   return current?{ok:true,json:async()=>current}:previous(url,request);
  };
 });
 await p.locator('#settingsButton').click();await p.waitForFunction(()=>stockPosts.length===1&&!document.getElementById('settingsButton').disabled);
 assert.deepEqual(await p.evaluate(()=>stockPosts),[{stocks:{provider:'yahoo',chartRange:'5d',watchlist:[{symbol:'PTT.BK',enabled:true}]}}]);
 assert.doesNotMatch(await p.locator('#stocksSelectionStored').textContent(),/ยังไม่ได้บันทึก/);
 assert.equal(await p.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true);
 await p.locator('#stocksProvider').scrollIntoViewIfNeeded();await p.screenshot({path:require('node:path').join(__dirname,'../.platformio/native-ui-regression/portal-stocks-320.png')});await p.close();
});
test('actual portal DOM previews both languages, reads back independent saves, and preserves form data',{timeout:30000},async()=>{
 const page=await open({language:'en'});
 assert.equal(await page.locator('html').getAttribute('lang'),'en');assert.equal(await page.title(),'Desk Duo setup');
 assert.equal(await page.locator('#language option[value=en]').textContent(),'English');
 assert.equal(await page.locator('#codexUsageMode option[value=weekly]').textContent(),'Weekly only');
 assert.equal(await page.locator('#displayButton').textContent(),'Save display settings');
 assert.equal(await page.locator('#firmwareSelected').textContent(),'No file selected');
 assert.equal(await page.locator('label[for=firmwareFile]').textContent(),'Choose application file');
 assert.match(await page.locator('#weatherDataStatus').textContent(),/Stale data.*0°C.*0%.*HTTPS connection failed/);
 assert.equal(await page.locator('#ssid').inputValue(),'ต้นฉบับ <data>');assert.equal(await page.locator('#weatherLabel').inputValue(),'กรุงเทพฯ');
 const text=await page.locator('body').innerText();
 // Only the user-provided SSID may contain Thai in the English fixed copy.
 assert.doesNotMatch(text.replaceAll('ต้นฉบับ <data>','').replaceAll('กรุงเทพฯ',''),/[\u0e00-\u0e7f]/);
 await page.screenshot({path:path.join(__dirname,'../.platformio/native-ui-regression/portal-language-en.png'),fullPage:true});
 await page.locator('#weatherLabel').fill('Untouched editor');await page.locator('#language').selectOption('th');
 assert.equal(await page.locator('html').getAttribute('lang'),'th');assert.match(await page.title(),/ตั้งค่า/);
 assert.equal(await page.locator('#displayButton').textContent(),'บันทึกการแสดงผล');
 assert.equal(await page.locator('#firmwareSelected').textContent(),'ยังไม่เลือกไฟล์');
 assert.match(await page.locator('#languageStored').textContent(),/อังกฤษ.*ยังไม่ได้บันทึก/);
 assert.match(await page.locator('#weatherDataStatus').textContent(),/ข้อมูลเก่า.*0°C.*0%.*HTTPS.*ไม่สำเร็จ/);
 assert.equal(await page.locator('#weatherLabel').inputValue(),'Untouched editor');
 assert.deepEqual(await page.evaluate(()=>posts),[]);
 await page.locator('#displayButton').click();await page.waitForFunction(()=>document.getElementById('displayStatus').textContent.includes('บันทึกการแสดงผลแล้ว'));
 assert.deepEqual(await page.evaluate(()=>posts),[{language:'th'}]);assert.equal(await page.locator('#codexUsageMode').inputValue(),'weekly');
 assert.doesNotMatch(await page.locator('#languageStored').textContent(),/ยังไม่ได้บันทึก/);
 await page.screenshot({path:path.join(__dirname,'../.platformio/native-ui-regression/portal-language-th.png'),fullPage:true});
 await page.locator('#codexUsageMode').selectOption('both');await page.locator('#displayButton').click();await page.waitForFunction(()=>posts.length===2);
 assert.deepEqual(await page.evaluate(()=>posts[1]),{codexUsageMode:'both'});assert.equal(await page.locator('#language').inputValue(),'th');
 await page.close();
});
test('actual DOM failed save/readback and config failure preserve honest saved-language status',{timeout:30000},async()=>{
 const page=await open({saveFails:true,configFails:true});
 await page.locator('#language').selectOption('en');assert.equal(await page.locator('#settingsButton').isDisabled(),true);
 assert.match(await page.locator('#status').textContent(),/Could not load settings/);
 await page.locator('#displayButton').click();await page.waitForFunction(()=>document.getElementById('displayStatus').textContent.includes('Could not save'));
 assert.match(await page.locator('#languageStored').textContent(),/Thai.*Not saved/);
 await page.locator('#language').selectOption('th');assert.match(await page.locator('#displayStatus').textContent(),/ยังบันทึกไม่ได้/);
 await page.close();
 const q=await open({reloadFails:true});await q.locator('#weatherLabel').fill('Unsaved editor');await q.locator('#language').selectOption('en');await q.locator('#codexUsageMode').selectOption('both');await q.locator('#displayButton').click();
 await q.waitForFunction(()=>document.getElementById('displayStatus').textContent.includes('readback failed'));
 assert.deepEqual(await q.evaluate(()=>posts),[{codexUsageMode:'both',language:'en'}]);
 assert.equal(await q.locator('#displayButton').isDisabled(),true);
 assert.equal(await q.locator('#languageStored').textContent(),'Saved language unknown');
 assert.equal(await q.locator('#language').inputValue(),'en');assert.equal(await q.locator('#codexUsageMode').inputValue(),'both');
 assert.equal(await q.locator('#weatherLabel').inputValue(),'Unsaved editor');
 await q.screenshot({path:path.join(__dirname,'../.platformio/native-ui-regression/portal-language-readback-failed-en.png'),fullPage:true});
 await q.locator('#language').selectOption('th');assert.match(await q.locator('#languageStored').textContent(),/ไม่ทราบ/);
 assert.doesNotMatch(await q.locator('#languageStored').textContent(),/ยังไม่ได้บันทึก/);
 await q.screenshot({path:path.join(__dirname,'../.platformio/native-ui-regression/portal-language-readback-failed-th.png'),fullPage:true});
 await q.evaluate(()=>document.getElementById('display').requestSubmit());assert.equal((await q.evaluate(()=>posts)).length,1);
 await q.close();
});

test('actual brightness DOM: range endpoints, sparse/combined saves, narrow layout and locale preservation',{timeout:30000},async()=>{
 for(const lang of ['en','th']){
  const page=await open({language:lang,width:320});
  const range=page.locator('#brightnessPercent');assert.equal(await range.getAttribute('min'),'5');assert.equal(await range.getAttribute('max'),'100');
  assert.equal(await range.inputValue(),'100');assert.match(await page.locator('#brightnessStored').textContent(),/100%/);
  await page.locator('#weatherLabel').fill('My กรุงเทพฯ label');await range.fill('5');await range.dispatchEvent('input');
  assert.equal(await page.locator('#brightnessValue').textContent(),'5%');assert.match(await page.locator('#brightnessStored').textContent(),/100%.*(Not saved|ยังไม่ได้บันทึก)/);
  const other=lang==='en'?'th':'en';await page.locator('#language').selectOption(other);assert.equal(await range.inputValue(),'5');assert.equal(await page.locator('#weatherLabel').inputValue(),'My กรุงเทพฯ label');
  await page.locator('#codexUsageMode').selectOption('both');await page.locator('#displayButton').click();await page.waitForFunction(()=>posts.length===1 && !document.getElementById('displayButton').disabled);
  assert.deepEqual(await page.evaluate(()=>posts),[{codexUsageMode:'both',language:other,brightnessPercent:5}]);
  await range.fill('100');await range.dispatchEvent('input');await page.locator('#displayButton').click();await page.waitForFunction(()=>posts.length===2 && !document.getElementById('displayButton').disabled);
  assert.deepEqual(await page.evaluate(()=>posts[1]),{brightnessPercent:100});
  assert.equal(await page.locator('#weatherLabel').inputValue(),'My กรุงเทพฯ label');
  await page.locator('#language').selectOption(lang);
  await page.locator('#displayButton').click();await page.waitForFunction(()=>posts.length===3 && !document.getElementById('displayButton').disabled);
  assert.doesNotMatch(await page.locator('#languageStored').textContent(),/Not saved|ยังไม่ได้บันทึก/);
  await page.locator('#display').scrollIntoViewIfNeeded();
  assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true);
  const hit=await range.boundingBox();assert.ok(hit.height>=44 && hit.x>=0 && hit.x+hit.width<=320);
  await page.screenshot({path:path.join(__dirname,'../.platformio/native-ui-regression/portal-brightness-'+lang+'-320.png')});
  await page.setViewportSize({width:900,height:1100});await page.locator('#display').scrollIntoViewIfNeeded();
  await page.screenshot({path:path.join(__dirname,'../.platformio/native-ui-regression/portal-brightness-'+lang+'-desktop.png')});await page.close();
 }
});
test('actual brightness DOM retains failed selection/saved value; validates readback before claiming current',{timeout:30000},async()=>{
 const p=await open({language:'en',saveFails:true});await p.locator('#brightnessPercent').fill('50');await p.locator('#brightnessPercent').dispatchEvent('input');await p.locator('#displayButton').click();await p.waitForFunction(()=>document.getElementById('displayStatus').textContent.includes('Could not save'));
 assert.match(await p.locator('#brightnessStored').textContent(),/100%.*Not saved/);assert.equal(await p.locator('#brightnessPercent').inputValue(),'50');await p.close();
 for(const display of [null,{brightnessPercent:undefined},{brightnessPercent:null},{brightnessPercent:true},{brightnessPercent:4},{brightnessPercent:101},{brightnessPercent:50.5},{brightnessPercent:'50'}]){
  const q=await open({language:'en',reloadFails:display===null});
  if(display)await q.evaluate(value=>{scenario.display=value;window.display={...window.display,...value};},display);
  await q.locator('#brightnessPercent').fill('5');await q.locator('#brightnessPercent').dispatchEvent('input');await q.locator('#weatherLabel').fill('Unsaved user data');
  // Invalid response on readback, after accepted POST, must invalidate all saved claims.
  if(display)await q.evaluate(value=>{const original=fetch;window.fetch=async(url,request={})=>{const r=await original(url,request);return url==='/api/display'&&!request.method?{ok:true,json:async()=>({...window.display,...value})}:r;};},display);
  await q.locator('#displayButton').click();await q.waitForFunction(()=>document.getElementById('displayStatus').textContent.includes('readback failed'));
  assert.equal(await q.locator('#brightnessStored').textContent(),'Saved brightness unknown');assert.equal(await q.locator('#languageStored').textContent(),'Saved language unknown');assert.equal(await q.locator('#displayButton').isDisabled(),true);
  assert.equal(await q.locator('#brightnessPercent').inputValue(),'5');assert.equal(await q.locator('#weatherLabel').inputValue(),'Unsaved user data');await q.close();
 }
});

test('actual Auto DOM keeps dirty editors and locale through status refresh; sparse save and both-language captures',{timeout:30000},async()=>{
 for(const lang of ['en','th']){
  const page=await open({language:lang,width:320,display:{brightnessAuto:true,brightnessPercent:51,brightnessTargetPercent:26,brightnessEffectivePercent:26,brightnessPhase:'night'}});
  assert.equal(await page.locator('#brightnessAuto').isChecked(),true);assert.equal(await page.locator('#brightnessDayStart').getAttribute('type'),'time');assert.equal(await page.locator('#brightnessNightStart').getAttribute('step'),'60');
  assert.match(await page.locator('#brightnessStored').textContent(),/51%/);assert.match(await page.locator('#brightnessEffective').textContent(),/26%.*26%/);
  await page.locator('#display').scrollIntoViewIfNeeded();assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true);
  await page.screenshot({path:path.join(__dirname,'../.platformio/native-ui-regression/portal-auto-'+lang+'-320.png')});
  await page.setViewportSize({width:900,height:1100});await page.locator('#display').scrollIntoViewIfNeeded();await page.screenshot({path:path.join(__dirname,'../.platformio/native-ui-regression/portal-auto-'+lang+'-desktop.png')});
  await page.locator('#brightnessPercent').fill('11');await page.locator('#brightnessPercent').dispatchEvent('input');await page.locator('#brightnessDayStart').fill('07:15');await page.locator('#brightnessNightStart').fill('19:45');await page.locator('#brightnessAuto').uncheck();
  const other=lang==='en'?'th':'en';await page.locator('#language').selectOption(other);await page.locator('#weatherLabel').fill('Keep กรุงเทพฯ');
  await page.evaluate(async()=>{Object.assign(display,{brightnessPercent:75,brightnessTargetPercent:38,brightnessEffectivePercent:38,brightnessPhase:'night'});await displayPollers.shift()();});
  assert.equal(await page.locator('#brightnessPercent').inputValue(),'11');assert.equal(await page.locator('#brightnessDayStart').inputValue(),'07:15');assert.equal(await page.locator('#brightnessNightStart').inputValue(),'19:45');assert.equal(await page.locator('#brightnessAuto').isChecked(),false);assert.equal(await page.locator('#language').inputValue(),other);assert.equal(await page.locator('html').getAttribute('lang'),other);assert.equal(await page.locator('#weatherLabel').inputValue(),'Keep กรุงเทพฯ');assert.deepEqual(await page.evaluate(()=>posts),[]);
  assert.match(await page.locator('#brightnessStored').textContent(),/75%/);assert.match(await page.locator('#brightnessEffective').textContent(),/38%.*38%/);
  await page.locator('#displayButton').click();await page.waitForFunction(()=>posts.length===1&&!document.getElementById('displayButton').disabled);
  assert.deepEqual(await page.evaluate(()=>posts),[{language:other,brightnessPercent:11,brightnessAuto:false,brightnessDayStart:'07:15',brightnessNightStart:'19:45'}]);
  await page.locator('#brightnessDayStart').fill('19:45');await page.locator('#displayButton').click();assert.equal(await page.evaluate(()=>posts.length),1);assert.match(await page.locator('#displayStatus').textContent(),/HH:MM/);
  await page.close();
 }
});
test('actual Auto DOM rejects stale GET after accepted POST, recovers unknown snapshot, and retains failed POST state',{timeout:30000},async()=>{
 const page=await open({language:'en'});
 await page.evaluate(()=>{
  const previous=fetch;let delayed=true;
  window.fetch=async(url,request={})=>{
   if(url==='/api/display'&&!request.method&&delayed){delayed=false;const snapshot={...display};return await new Promise(resolve=>window.releaseDisplay=()=>resolve({ok:true,json:async()=>snapshot}));}
   return previous(url,request);
  };
  window.polling=displayPollers.shift()();
 });
 await page.locator('#brightnessAuto').check();await page.locator('#brightnessPercent').fill('11');await page.locator('#brightnessPercent').dispatchEvent('input');
 await page.locator('#displayButton').click();await page.waitForFunction(()=>document.getElementById('brightnessStored').textContent==='Saved brightness unknown');
 await page.evaluate(()=>{scenario.reloadFails=true;releaseDisplay();});await page.waitForFunction(()=>document.getElementById('displayStatus').textContent.includes('readback failed'));
 assert.equal(await page.locator('#brightnessStored').textContent(),'Saved brightness unknown');assert.equal(await page.locator('#displayButton').isDisabled(),true);assert.equal(await page.locator('#brightnessPercent').inputValue(),'11');assert.equal(await page.locator('#brightnessAuto').isChecked(),true);
 await page.evaluate(async()=>{scenario.reloadFails=false;await displayPollers.shift()();});assert.match(await page.locator('#brightnessStored').textContent(),/11%/);assert.equal(await page.locator('#displayButton').isDisabled(),false);assert.match(await page.locator('#brightnessEffective').textContent(),/Last applied Unknown/);
 await page.evaluate(()=>{scenario.saveFails=true;});await page.locator('#brightnessNightStart').fill('20:30');await page.locator('#displayButton').click();await page.waitForFunction(()=>document.getElementById('displayStatus').textContent.includes('Could not save'));
 assert.match(await page.locator('#brightnessScheduleStored').textContent(),/18:00.*Not saved/);assert.equal(await page.locator('#brightnessNightStart').inputValue(),'20:30');
 await page.evaluate(async()=>{scenario.reloadFails=true;await displayPollers.shift()();});assert.equal(await page.locator('#brightnessEffective').textContent(),'Brightness status: Unknown');
 await page.locator('#brightnessPercent').fill('51');await page.locator('#brightnessPercent').dispatchEvent('input');assert.equal(await page.locator('#brightnessEffective').textContent(),'Brightness status: Unknown');
 await page.locator('#language').selectOption('th');assert.match(await page.locator('#brightnessEffective').textContent(),/ไม่ทราบ/);
 await page.evaluate(async()=>{scenario.reloadFails=false;await displayPollers.shift()();});assert.match(await page.locator('#brightnessEffective').textContent(),/100%/);assert.equal(await page.locator('#brightnessPercent').inputValue(),'51');await page.close();
});

test('actual DOM rebases clean concurrent schedule after POST, keeps pending editors, and preserves sparse next save',{timeout:30000},async()=>{
 const page=await open({language:'en'});
 await page.evaluate(()=>{display.brightnessDayStart='07:15';});await page.locator('#brightnessPercent').fill('50');await page.locator('#brightnessPercent').dispatchEvent('input');await page.locator('#displayButton').click();await page.waitForFunction(()=>posts.length===1&&!document.getElementById('displayButton').disabled);
 assert.deepEqual(await page.evaluate(()=>posts),[{brightnessPercent:50}]);assert.equal(await page.locator('#brightnessDayStart').inputValue(),'07:15');assert.doesNotMatch(await page.locator('#brightnessScheduleStored').textContent(),/Not saved/);
 await page.locator('#brightnessPercent').fill('60');await page.locator('#brightnessPercent').dispatchEvent('input');await page.locator('#displayButton').click();await page.waitForFunction(()=>posts.length===2&&!document.getElementById('displayButton').disabled);assert.deepEqual(await page.evaluate(()=>posts[1]),{brightnessPercent:60});assert.equal(await page.evaluate(()=>display.brightnessDayStart),'07:15');
 await page.evaluate(()=>{
  Object.assign(display,{brightnessDayStart:'08:30',brightnessNightStart:'20:00',codexUsageMode:'both'});const previous=fetch;let delay=true;
  window.fetch=async(url,request={})=>{
   if(url==='/api/display'&&!request.method&&delay){delay=false;return new Promise(resolve=>window.releaseReadback=()=>resolve({ok:true,json:async()=>({...display})}));}
   return previous(url,request);
  };
 });
 await page.locator('#brightnessPercent').fill('70');await page.locator('#brightnessPercent').dispatchEvent('input');await page.locator('#displayButton').click();await page.waitForFunction(()=>document.getElementById('brightnessStored').textContent==='Saved brightness unknown');assert.equal(await page.locator('#displayButton').isDisabled(),true);
 await page.locator('#brightnessPercent').fill('80');await page.locator('#brightnessPercent').dispatchEvent('input');await page.locator('#brightnessNightStart').fill('21:45');await page.locator('#language').selectOption('th');await page.locator('#weatherLabel').fill('Keep user data');await page.evaluate(()=>releaseReadback());await page.waitForFunction(()=>!document.getElementById('displayButton').disabled);
 assert.equal(await page.locator('#brightnessDayStart').inputValue(),'08:30');assert.equal(await page.locator('#codexUsageMode').inputValue(),'both');assert.equal(await page.locator('#brightnessPercent').inputValue(),'80');assert.equal(await page.locator('#brightnessNightStart').inputValue(),'21:45');assert.equal(await page.locator('html').getAttribute('lang'),'th');assert.equal(await page.locator('#weatherLabel').inputValue(),'Keep user data');assert.match(await page.locator('#brightnessStored').textContent(),/70%/);
 await page.locator('#displayButton').click();await page.waitForFunction(()=>posts.length===4&&!document.getElementById('displayButton').disabled);assert.deepEqual(await page.evaluate(()=>posts[2]),{brightnessPercent:70});assert.deepEqual(await page.evaluate(()=>posts[3]),{language:'th',brightnessPercent:80,brightnessNightStart:'21:45'});assert.equal(await page.evaluate(()=>display.brightnessDayStart),'08:30');await page.close();
});

test('actual DOM preserves edits and locale on first valid recovery after initial failure or invalid readback',{timeout:30000},async()=>{
 for(const initial of [{initialDisplayFails:true},{display:{brightnessAuto:null}}]){
  const page=await open(initial);assert.equal(await page.locator('#displayButton').isDisabled(),true);
  await page.locator('#language').selectOption('en');await page.locator('#brightnessPercent').fill('11');await page.locator('#brightnessPercent').dispatchEvent('input');await page.locator('#brightnessDayStart').fill('07:15');await page.locator('#weatherLabel').fill('Keep user data');
  assert.equal(await page.locator('#brightnessStored').textContent(),'Saved brightness unknown');await page.evaluate(()=>document.getElementById('display').requestSubmit());assert.deepEqual(await page.evaluate(()=>posts),[]);
  await page.evaluate(async()=>{scenario.initialDisplayFails=false;Object.assign(display,{brightnessAuto:true,brightnessNightStart:'20:30'});await displayPollers.shift()();});
  assert.equal(await page.locator('#language').inputValue(),'en');assert.equal(await page.locator('html').getAttribute('lang'),'en');assert.equal(await page.locator('#brightnessPercent').inputValue(),'11');assert.equal(await page.locator('#brightnessDayStart').inputValue(),'07:15');assert.equal(await page.locator('#weatherLabel').inputValue(),'Keep user data');
  assert.equal(await page.locator('#codexUsageMode').inputValue(),'weekly');assert.equal(await page.locator('#brightnessAuto').isChecked(),true);assert.equal(await page.locator('#brightnessNightStart').inputValue(),'20:30');assert.equal(await page.locator('#displayButton').isDisabled(),false);assert.match(await page.locator('#brightnessStored').textContent(),/100%.*Not saved/);
  await page.locator('#displayButton').click();await page.waitForFunction(()=>posts.length===1&&!document.getElementById('displayButton').disabled);assert.deepEqual(await page.evaluate(()=>posts),[{language:'en',brightnessPercent:11,brightnessDayStart:'07:15'}]);await page.close();
 }
});
test('actual DOM preserves edits made during the first pending GET and then saves only those fields',{timeout:30000},async()=>{
 const page=await open({firstDisplayPending:true,display:{brightnessAuto:true,brightnessNightStart:'20:30'}});assert.equal(await page.locator('#displayButton').isDisabled(),true);
 await page.locator('#language').selectOption('en');await page.locator('#brightnessPercent').fill('11');await page.locator('#brightnessPercent').dispatchEvent('input');await page.locator('#brightnessDayStart').fill('07:15');
 await page.evaluate(()=>releaseInitialDisplay());await page.waitForFunction(()=>!document.getElementById('displayButton').disabled);
 assert.equal(await page.locator('#language').inputValue(),'en');assert.equal(await page.locator('html').getAttribute('lang'),'en');assert.equal(await page.locator('#brightnessPercent').inputValue(),'11');assert.equal(await page.locator('#brightnessDayStart').inputValue(),'07:15');assert.equal(await page.locator('#brightnessNightStart').inputValue(),'20:30');assert.equal(await page.locator('#brightnessAuto').isChecked(),true);assert.equal(await page.locator('#codexUsageMode').inputValue(),'weekly');assert.deepEqual(await page.evaluate(()=>posts),[]);
 await page.locator('#displayButton').click();await page.waitForFunction(()=>posts.length===1&&!document.getElementById('displayButton').disabled);assert.deepEqual(await page.evaluate(()=>posts),[{language:'en',brightnessPercent:11,brightnessDayStart:'07:15'}]);await page.close();
});

test('six real sections retain drafts and scope configuration saves at 320px',{timeout:30000},async()=>{
 const p=await open({language:'en',width:320,navigation:true});
 assert.equal(await p.locator('#sectionSelect option').count(),6);
 await p.locator('#sectionSelect').selectOption('weather');await p.locator('#weatherLabel').fill('Draft location');
 await p.locator('#sectionSelect').selectOption('wifi');await p.locator('#ssid').fill('New network');
 await p.locator('#sectionSave').click();await p.waitForFunction(()=>configPosts.length===1);
 assert.deepEqual(await p.evaluate(()=>configPosts),[{ssid:'New network'}]);
 await p.locator('#sectionSelect').selectOption('weather');assert.equal(await p.locator('#weatherLabel').inputValue(),'Draft location');
 await p.locator('#sectionDiscard').click();assert.equal(await p.locator('#weatherLabel').inputValue(),'กรุงเทพฯ');
 for(const section of ['display','usage','weather','stocks','wifi','firmware']){
  await p.locator('#sectionSelect').selectOption(section);
  assert.equal(await p.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true);
 }
 await p.screenshot({path:path.join(__dirname,'../.platformio/native-ui-regression/portal-unified-mobile.png'),fullPage:true});
 await p.setViewportSize({width:1100,height:1000});await p.locator('[data-section="display"]').click();
 await p.screenshot({path:path.join(__dirname,'../.platformio/native-ui-regression/portal-unified-desktop.png'),fullPage:true});await p.close();
});

test('section saves isolate display API fields and retain other invalid drafts',{timeout:30000},async()=>{
 const p=await open({language:'en',width:320,navigation:true});
 await p.locator('#sectionSelect').selectOption('weather');await p.locator('#section-weather summary').filter({hasText:'Advanced'}).click();await p.locator('#weatherHeaders').fill('{invalid draft');
 await p.locator('#sectionSelect').selectOption('usage');await p.locator('#codexUsageMode').selectOption('both');
 await p.locator('#sectionSelect').selectOption('display');await p.locator('#brightnessPercent').fill('55');await p.locator('#sectionSave').click();
 await p.waitForFunction(()=>posts.length===1&&!document.getElementById('displayButton').disabled);
 assert.deepEqual(await p.evaluate(()=>posts),[{brightnessPercent:55}]);
 await p.locator('#sectionSelect').selectOption('usage');assert.equal(await p.locator('#codexUsageMode').inputValue(),'both');
 await p.locator('#sectionSave').click();await p.waitForFunction(()=>posts.length===2);assert.deepEqual(await p.evaluate(()=>posts[1]),{codexUsageMode:'both'});
 await p.locator('#sectionSelect').selectOption('weather');assert.equal(await p.locator('#weatherHeaders').inputValue(),'{invalid draft');await p.close();
});

test('usage mode saves independently of an invalid unsaved Display schedule',{timeout:30000},async()=>{
 const p=await open({language:'en',width:320,navigation:true});
 await p.locator('#brightnessDayStart').fill('18:00');
 await p.locator('#sectionSelect').selectOption('usage');await p.locator('#codexUsageMode').selectOption('both');await p.locator('#sectionSave').click();
 await p.waitForFunction(()=>posts.length||document.getElementById('displayStatus').textContent.includes('HH:MM'));
 assert.deepEqual(await p.evaluate(()=>posts),[{codexUsageMode:'both'}]);
 await p.locator('#sectionSelect').selectOption('display');assert.equal(await p.locator('#brightnessDayStart').inputValue(),'18:00');await p.close();
});

test('Open-Meteo save rejects invalid or missing friendly coordinates without sending an older URL',{timeout:30000},async()=>{
 const p=await open({language:'en',width:320,navigation:true});await p.locator('#sectionSelect').selectOption('weather');
 await p.locator('#weatherLatitude').fill('13');await p.locator('#weatherLongitude').fill('100');await p.locator('#weatherProvider').selectOption('open-meteo');
 const generated=await p.locator('#weatherUrl').inputValue();
 for(const [id,value] of [['weatherLatitude','91'],['weatherLatitude','-91'],['weatherLongitude','181'],['weatherLatitude',''],['weatherLongitude','']]){
  await p.locator('#weatherLatitude').fill('13');await p.locator('#weatherLongitude').fill('100');await p.locator('#'+id).fill(value);await p.locator('#sectionSave').click();
  assert.deepEqual(await p.evaluate(()=>configPosts),[]);assert.equal(await p.locator('#'+id).inputValue(),value);
  assert.match(await p.locator('#status').textContent(),/Coordinates require valid/);
 }
 await p.locator('#weatherLatitude').fill('13');await p.locator('#weatherLongitude').fill('100');await p.locator('#weatherLongitude').dispatchEvent('change');
 const url=new URL(await p.locator('#weatherUrl').inputValue());assert.match(url.searchParams.get('hourly'),/precipitation_probability/);
 assert.equal(JSON.parse(await p.locator('#weatherFields').inputValue()).hourlyRainProbability,'hourly.precipitation_probability');assert.ok(generated);await p.close();
});

test('friendly weather edits derive complete forecasts only after explicit provider choice',{timeout:30000},async()=>{
 const p=await open({language:'en',width:320,navigation:true});await p.locator('#sectionSelect').selectOption('weather');
 assert.equal(await p.locator('#weatherProvider').inputValue(),'custom');assert.equal(await p.locator('#weatherUrl').inputValue(),'');
 await p.locator('#weatherLatitude').fill('13.7563');await p.locator('#weatherLongitude').fill('100.5018');await p.locator('#weatherLongitude').dispatchEvent('change');
 assert.equal(await p.locator('#weatherUrl').inputValue(),'');
 await p.locator('#weatherProvider').selectOption('open-meteo');
 const url=new URL(await p.locator('#weatherUrl').inputValue());assert.equal(url.searchParams.get('latitude'),'13.7563');assert.equal(url.searchParams.get('longitude'),'100.5018');assert.match(url.searchParams.get('current'),/is_day/);assert.match(url.searchParams.get('hourly'),/weather_code,is_day/);
 const fields=JSON.parse(await p.locator('#weatherFields').inputValue());assert.equal(fields.isDay,'current.is_day');assert.equal(fields.hourlyWeatherCodes,'hourly.weather_code');
 await p.locator('#sectionSave').click();await p.waitForFunction(()=>configPosts.length===1);
 assert.equal((await p.evaluate(()=>configPosts))[0].weather.url,url.href);assert.deepEqual(await p.evaluate(()=>posts),[]);await p.close();
});

test('production redacted Weather GET hydrates safe coordinates and preserves secret URL on scoped saves',{timeout:30000},async()=>{
 const privateWeatherUrl='https://api.open-meteo.com/v1/forecast?latitude=13.7563&longitude=100.5018&apikey=hidden-key';
 const publicConfig=JSON.parse(require('node:child_process').execFileSync(path.join(__dirname,'../.platformio/native-tests/firmware_core_test'),['--public-weather-config',privateWeatherUrl],{encoding:'utf8'}));
 assert.equal(publicConfig.weather.url,'https://api.open-meteo.com/v1/forecast');assert.equal(JSON.stringify(publicConfig).includes('hidden-key'),false);
 const p=await open({language:'en',width:320,navigation:true,publicConfig,privateWeatherUrl});await p.locator('#sectionSelect').selectOption('weather');
 assert.equal(await p.locator('#weatherLatitude').inputValue(),'13.7563');assert.equal(await p.locator('#weatherLongitude').inputValue(),'100.5018');assert.equal(await p.locator('#weatherUrl').inputValue(),'');
 await p.locator('#weatherLabel').fill('New label');await p.locator('#sectionSave').click();await p.waitForFunction(()=>configPosts.length===1);
 assert.deepEqual(await p.evaluate(()=>configPosts[0]),{weather:{label:'New label'}});assert.equal(await p.evaluate(()=>privateWeatherUrl),privateWeatherUrl);
 await p.evaluate(()=>load());await p.locator('#weatherRefresh').fill('1200');await p.locator('#sectionSave').click();await p.waitForFunction(()=>configPosts.length===2);
 assert.deepEqual(await p.evaluate(()=>configPosts[1]),{weather:{refreshSeconds:1200}});assert.equal(await p.evaluate(()=>privateWeatherUrl),privateWeatherUrl);assert.equal(await p.locator('#weatherUrl').inputValue(),'');await p.close();
});
test('legacy redacted Weather GET permits unchanged source edits but requires coordinates after explicit friendly edits',{timeout:30000},async()=>{
 const p=await open({language:'en',width:320,navigation:true});
 await p.evaluate(async()=>{configSnapshot.weather.url='https://api.open-meteo.com/v1/forecast';configSnapshot.weather.urlDetailsHidden=true;delete configSnapshot.weather.latitude;delete configSnapshot.weather.longitude;await load();});
 await p.locator('#sectionSelect').selectOption('weather');assert.equal(await p.locator('#weatherLatitude').inputValue(),'');
 await p.locator('#weatherFields').evaluate(node=>node.closest('details').open=true);await p.locator('#weatherLabel').fill('Legacy label');await p.locator('#weatherRefresh').fill('1200');await p.locator('#weatherFields').fill('{"temperature":"current.temp"}');await p.locator('#sectionSave').click();
 await p.waitForFunction(()=>configPosts.length||document.getElementById('status').textContent.includes('Coordinates require'));
 assert.deepEqual(await p.evaluate(()=>configPosts),[{weather:{label:'Legacy label',refreshSeconds:1200,fields:{temperature:'current.temp'}}}]);
 await p.locator('#weatherLatitude').fill('91');await p.locator('#weatherLatitude').dispatchEvent('change');await p.locator('#weatherLabel').fill('Blocked');await p.locator('#sectionSave').click();assert.equal(await p.evaluate(()=>configPosts.length),1);assert.match(await p.locator('#status').textContent(),/Coordinates require/);
 await p.locator('#sectionDiscard').click();await p.locator('#weatherProvider').selectOption('custom');await p.locator('#weatherProvider').selectOption('open-meteo');await p.locator('#weatherLabel').fill('Still blocked');await p.locator('#sectionSave').click();assert.equal(await p.evaluate(()=>configPosts.length),1);assert.match(await p.locator('#status').textContent(),/Coordinates require/);await p.close();
});

test('firmware review shows real size before any binary request',{timeout:30000},async()=>{
 const p=await open({language:'en',width:320,navigation:true});await p.locator('#sectionSelect').selectOption('firmware');
 await p.evaluate(()=>{window.binaryPosts=0;window.XMLHttpRequest=class{constructor(){this.upload={};}open(){}setRequestHeader(){}send(){binaryPosts++;}};});
 await p.locator('#firmwareFile').setInputFiles({name:'desk-duo-app.bin',mimeType:'application/octet-stream',buffer:Buffer.alloc(2048)});
 assert.match(await p.locator('#firmwareSelected').textContent(),/2048 bytes/);await p.locator('#firmwareButton').click();
 await p.waitForFunction(()=>!document.getElementById('firmwareReview').hidden);assert.match(await p.locator('#firmwareReview').textContent(),/2048 bytes/);assert.equal(await p.evaluate(()=>binaryPosts),0);
 await p.locator('#firmwareButton').click();await p.waitForFunction(()=>binaryPosts===1);await p.close();
});

test('approved identity thumbnails and weather artwork render offline from actual condition flags',{timeout:30000},async()=>{
 const p=await open({language:'th',width:320,navigation:true,epochSeconds:1792702800,weather:{weatherCode:2,isDay:false,updatedAt:1792616400,utcOffsetSeconds:25200,stale:false,ageMs:0}});
 await p.locator('#sectionSelect').selectOption('usage');
 assert.equal(await p.locator('#claudeIdentity').count(),1);assert.equal(await p.locator('#codexIdentity').count(),1);
 for(const id of ['claudeIdentity','codexIdentity'])assert.equal(await p.locator('#'+id).evaluate(img=>img.complete&&img.naturalWidth===64),true);
 await p.screenshot({path:path.join(__dirname,'../.platformio/native-ui-regression/portal-approved-usage-th-320.png'),fullPage:true});
 await p.locator('#sectionSelect').selectOption('weather');assert.equal(await p.locator('#weatherArtwork').getAttribute('data-tile'),'3');assert.match(await p.locator('#weatherDataStatus').textContent(),/ข้อมูลเก่า/);
 assert.equal(await p.locator('#weatherArtwork').evaluate(img=>img.complete&&img.naturalWidth===80),true);
 await p.screenshot({path:path.join(__dirname,'../.platformio/native-ui-regression/portal-approved-weather-th-320.png'),fullPage:true});
 await p.locator('#sectionSelect').selectOption('display');await p.locator('#language').selectOption('en');await p.locator('#sectionSelect').selectOption('weather');
 assert.equal(await p.locator('#weatherArtwork').getAttribute('data-tile'),'3');assert.equal(await p.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true);
 await p.setViewportSize({width:1100,height:1000});await p.screenshot({path:path.join(__dirname,'../.platformio/native-ui-regression/portal-approved-weather-en-desktop.png'),fullPage:true});
 for(const [code,isDay,tile] of [[1,true,'0'],[2,true,'1'],[0,false,'2'],[3,null,'4'],[61,null,'5']]){await p.evaluate(([weatherCode,isDay])=>{scenario.weather={weatherCode,isDay};},[code,isDay]);await p.locator('#networkRefresh').click();await p.waitForFunction(tile=>document.getElementById('weatherArtwork').dataset.tile===tile,tile);}
 await p.close();
 const unknown=await open({language:'en',navigation:true,weather:{weatherCode:0,isDay:null}});await unknown.locator('[data-section="weather"]').click();assert.equal(await unknown.locator('#weatherArtwork').isHidden(),true);await unknown.close();
});

test('failed scoped save retains saved state and draft; TH and EN navigation fit 320px',{timeout:30000},async()=>{
 for(const language of ['en','th']){
  const p=await open({language,width:320,navigation:true,configSaveFails:true});
  await p.locator('#sectionSelect').selectOption('wifi');await p.locator('#ssid').fill('Uncommitted');await p.locator('#sectionSave').click();
  await p.waitForFunction(()=>configPosts.length===1);assert.equal(await p.locator('#ssid').inputValue(),'Uncommitted');assert.match(await p.locator('#sectionState').textContent(),/ต้นฉบับ/);assert.match(await p.locator('#status').textContent(),/Maintenance|บันทึกไม่สำเร็จ/);
  await p.locator('#sectionSelect').selectOption('display');await p.locator('#language').selectOption(language==='en'?'th':'en');
  await p.locator('#sectionSelect').selectOption('wifi');assert.equal(await p.locator('#ssid').inputValue(),'Uncommitted');
  await p.locator('#sectionSelect').selectOption('display');await p.locator('#language').selectOption(language);
  for(const section of ['display','usage','weather','stocks','wifi','firmware']){await p.locator('#sectionSelect').selectOption(section);assert.equal(await p.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true);await p.screenshot({path:path.join(__dirname,'../.platformio/native-ui-regression/portal-unified-'+section+'-'+language+'-320.png'),fullPage:true});}
  await p.close();
 }
});
