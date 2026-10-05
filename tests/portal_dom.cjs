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
 const page=await browser.newPage({viewport:{width:720,height:1280}});
 await page.route('**/*',route=>route.abort());
 await page.evaluate(options=>{
  window.scenario=options;window.posts=[];window.display={codexUsageMode:'weekly',language:options.language||'th'};window.displayLoads=0;
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
 await page.setContent(html);await page.waitForFunction(()=>!document.getElementById('displayButton').disabled);
 return page;
}
test('actual portal DOM previews both languages, reads back independent saves, and preserves form data',async()=>{
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
test('actual DOM failed save/readback and config failure preserve honest saved-language status',async()=>{
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
