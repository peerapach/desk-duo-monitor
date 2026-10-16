// Exercise the production HTML in a real offline Chromium DOM. Run with the
// existing Playwright runtime via NODE_PATH; no page or dependency is fetched.
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
  window.scenario=options;window.posts=[];window.display={codexUsageMode:'weekly',language:options.language||'th',brightnessPercent:100,brightnessAuto:false,brightnessDayStart:'06:00',brightnessNightStart:'18:00',brightnessTargetPercent:100,brightnessEffectivePercent:null,brightnessPhase:'manual',...options.display};window.displayLoads=0;
  window.fetch=async(url,request={})=>{
   if(url==='/api/display'){
    if(request.method){const patch=JSON.parse(request.body);posts.push(patch);if(!scenario.saveFails)Object.assign(display,patch);return {ok:!scenario.saveFails,status:scenario.saveFails?503:204};}
    if(++displayLoads>1 && scenario.reloadFails)throw Error('private readback failure');
    return {ok:true,json:async()=>({...display})};
   }
   if(url==='/api/firmware')return {ok:true,json:async()=>({version:'2.0.0',capacity:1966080,busy:false,runningPartition:'ota_0'})};
   if(url==='/api/network')return {ok:true,json:async()=>({stationOnline:true,weatherConfigured:true,weather:{hasData:true,stale:true,temperature:0,humidity:0,weatherCode:3,hourlyCount:4,ageMs:61000},attempt:{attempted:true,currentGeneration:true,outcome:'tls_failure'}})};
   if(url==='/api/config'){
    if(scenario.configFails)throw Error('private config failure');
    return {ok:true,json:async()=>({currentSSID:'ต้นฉบับ <data>',wifiPasswordConfigured:true,usageTokenConfigured:true,autoRotateSeconds:20,
     touch:{swapAxes:false,invertX:true,invertY:false,minX:294,maxX:3766,minY:248,maxY:3632},
     weather:{url:'https://weather.test/data',headersConfigured:true,urlDetailsHidden:true,label:'กรุงเทพฯ',fields:{temperature:'current.temperature_2m'},refreshSeconds:900},
     stocks:{url:'https://stocks.test/feed',headersConfigured:false,urlDetailsHidden:false,refreshSeconds:300}})};
   }
   throw Error('Unexpected fetch '+url);
  };
 },options);
 await page.setContent(html);await page.waitForFunction(()=>document.getElementById('displayStatus').textContent || !document.getElementById('displayButton').disabled);
 return page;
}
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
 assert.doesNotMatch(text.replace('ต้นฉบับ <data>',''),/[\u0e00-\u0e7f]/);
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
