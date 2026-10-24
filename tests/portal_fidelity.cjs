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
 const page=await browser.newPage({viewport:{width:options.width||720,height:1000}});
 page.on('pageerror',e=>{throw e;});await page.route('**/*',route=>route.abort());
 await page.evaluate(options=>{
  window.displayPollers=[];const nativeTimeout=window.setTimeout;
  window.setTimeout=(fn,ms,...args)=>ms===15000?(displayPollers.push(fn),-displayPollers.length):nativeTimeout(fn,ms,...args);
  window.scenario=options;window.configPosts=[];window.configSnapshot=options.publicConfig||null;window.privateWeatherUrl=options.privateWeatherUrl;window.posts=[];window.display={codexUsageMode:'weekly',language:options.language||'th',brightnessPercent:100,brightnessAuto:false,brightnessDayStart:'06:00',brightnessNightStart:'18:00',brightnessTargetPercent:100,brightnessEffectivePercent:null,brightnessPhase:'manual',...options.display};window.displayLoads=0;
  window.fetch=async(url,request={})=>{
   if(url==='/api/dashboard')return {ok:!!options.dashboard,status:options.dashboard?200:404,json:async()=>options.dashboard};
   if(url==='/api/display'){
    if(request.method){const patch=JSON.parse(request.body);posts.push(patch);if(!scenario.saveFails)Object.assign(display,patch);return {ok:!scenario.saveFails,status:scenario.saveFails?503:204};}
    if(scenario.firstDisplayPending && !displayLoads++){return await new Promise(resolve=>window.releaseInitialDisplay=()=>resolve({ok:true,json:async()=>({...display})}));}
    if(scenario.initialDisplayFails && !displayLoads)throw Error('initial display failure');
    if(++displayLoads>1 && scenario.reloadFails)throw Error('private readback failure');
    return {ok:true,json:async()=>({...display})};
   }
   if(url==='/api/firmware')return {ok:true,json:async()=>({version:'2.0.0',capacity:1966080,busy:false,runningPartition:'ota_0'})};
   if(url==='/api/network')return {ok:true,json:async()=>({stationOnline:true,weatherConfigured:true,epochSeconds:options.epochSeconds,weather:{hasData:true,stale:true,temperature:0,humidity:0,weatherCode:3,hourlyCount:4,ageMs:61000,...options.weather},attempt:{attempted:true,currentGeneration:true,outcome:'accepted'}})};
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
 await page.waitForFunction(()=>displayLoaded&&!!saved);await page.waitForTimeout(50);await page.evaluate(()=>{const mark=document.createElement('span');mark.textContent='FIXTURE';mark.style='font-size:9px;letter-spacing:.7px;color:#94aaa0';mark.title='Offline test API fixtures; production HTML';document.querySelector('.header-tools').prepend(mark);});
 return page;
}

const fixture={publicConfig:{currentSSID:'Desk-WiFi',wifiPasswordConfigured:true,usageTokenConfigured:true,autoRotateSeconds:0,touch:{swapAxes:false,invertX:false,invertY:false,minX:0,maxX:4095,minY:0,maxY:4095},weather:{url:'https://api.open-meteo.com/v1/forecast',urlDetailsHidden:true,latitude:13.7563,longitude:100.5018,headersConfigured:false,label:'Bangkok',refreshSeconds:900,fields:{temperature:'current.temperature_2m',isDay:'current.is_day'}},stocks:{provider:'yahoo',url:'',headersConfigured:false,urlDetailsHidden:false,refreshSeconds:300,chartRange:'1d',watchlist:[{symbol:'PTT.BK',enabled:true},{symbol:'ADVANC.BK',enabled:true},{symbol:'AAPL',enabled:true},{symbol:'MSFT',enabled:true}]}},display:{brightnessPercent:60,brightnessAuto:true,codexUsageMode:'both',brightnessTargetPercent:60,brightnessEffectivePercent:60,brightnessPhase:'day'},weather:{hasData:true,stale:false,temperature:29.6,humidity:81,weatherCode:1,isDay:null,updatedAt:1792735200,utcOffsetSeconds:25200},epochSeconds:1792736100,dashboard:{claude:{hasData:true,stale:false,primary:{usedPercent:38,windowMinutes:300,resetsAt:null},secondary:{usedPercent:64,windowMinutes:10080,resetsAt:null}},codex:{hasData:true,stale:false,primary:{usedPercent:21,windowMinutes:300,resetsAt:null},secondary:{usedPercent:47,windowMinutes:10080,resetsAt:null}},stock:{hasData:true,stale:false,symbol:'AAPL',currency:'USD',price:224.80,changePercent:0.84,quotedAt:1792736100,chartRange:'1d',history:[219,220.1,219.7,221.4,220.7,222,222.3,221.8,223.6,224.8]}}};
test('production cards show actual fixture DTO data and legible controls at 1100/390/320 in TH and EN',{timeout:120000},async()=>{
 for(const width of [1100,390,320])for(const language of ['en','th']){
  const p=await open({...fixture,width,language,navigation:true});
  for(const section of ['display','usage','weather','stocks','wifi','firmware']){
   await p.evaluate(name=>selectSection(name),section);await p.waitForTimeout(50);
   const metrics=await p.evaluate(name=>{const s=document.getElementById('section-'+name),e=s.querySelector('.editor-column'),a=s.querySelector('aside');return {overflow:document.documentElement.scrollWidth>innerWidth,head:s.querySelector('h2').getBoundingClientRect().bottom,editor:e.getBoundingClientRect().top,left:e.getBoundingClientRect().left,aside:a.getBoundingClientRect().left,cards:e.querySelectorAll('.card').length,icons:document.querySelectorAll('.sidebar svg').length};},section);
   assert.equal(metrics.overflow,false,section+' overflow '+width);assert.equal(metrics.icons,6);assert.ok(metrics.head<metrics.editor);assert.ok(metrics.cards>=2);if(width===1100)assert.ok(metrics.aside>metrics.left+300);
   if(section==='usage'){assert.match(await p.locator('#claudeMeters').innerText(),/38%/);assert.match(await p.locator('#codexMeters').innerText(),/47%/);assert.equal(await p.locator('#codexMeters .meter i').first().evaluate(n=>n.style.width),'21%');}
   if(section==='weather'){assert.equal(await p.locator('#weatherTemperature').innerText(),'29.6°C');assert.match(await p.locator('#weatherHumidity').innerText(),/81%/);assert.equal(await p.locator('#weatherPlace').innerText(),'Bangkok');assert.equal(await p.locator('#neutralWeather').isVisible(),true);assert.equal(await p.locator('#weatherArtwork').isVisible(),false);}
   if(section==='stocks'){assert.equal(await p.locator('#quotePrice').innerText(),'224.80');assert.equal(await p.locator('#quoteSymbol').innerText(),'AAPL');assert.match(await p.locator('#quotePath').getAttribute('d'),/^M.+L/);assert.equal(await p.locator('#stockMarket0').evaluate(n=>{const c=getComputedStyle(n),ctx=document.createElement('canvas').getContext('2d');ctx.font=c.font;return n.clientWidth-parseFloat(c.paddingLeft)-parseFloat(c.paddingRight)>=ctx.measureText('TH').width+18&&n.getBoundingClientRect().height>=44;}),true);assert.ok(await p.locator('.stock-editor label:last-child').first().evaluate(n=>getComputedStyle(n).whiteSpace==='nowrap'&&n.getBoundingClientRect().height>=44));}
   if(section==='wifi')assert.equal(await p.locator('#connectionName').innerText(),'Desk-WiFi');if(section==='firmware')assert.equal(await p.locator('#firmwareProgress').isVisible(),false);
   await p.screenshot({path:path.join(__dirname,`../.worktrees/fidelity-fix/admin-fixture-${section}-${width}-${language}.png`),fullPage:true});
  }
  await p.evaluate(()=>selectSection('stocks'));await p.locator('#stockSymbol0').fill('SCB');await p.locator('#stocksRange').selectOption('5d');assert.equal(await p.locator('#quoteSymbol').innerText(),'AAPL');assert.match(await p.locator('#quoteStatus').innerText(),/1d/);assert.equal(await p.locator('#draft-stocks').getAttribute('data-dirty'),'true');await p.locator('#header'+(language==='en'?'TH':'EN')).click();assert.equal(await p.locator('#stockSymbol0').inputValue(),'SCB');assert.equal(await p.evaluate(()=>configPosts.length+posts.length),0);assert.equal(await p.locator('#language').inputValue(),language);await p.close();
 }
});
test('absent dashboard has honest unknown values and leaves saving usable',{timeout:20000},async()=>{const p=await open({...fixture,dashboard:undefined,navigation:true,language:'en'});await p.evaluate(()=>selectSection('usage'));await p.waitForTimeout(30);assert.match(await p.locator('#claudeMeters').innerText(),/No live data/);assert.equal(await p.locator('#claudeMeters .meter i').count(),0);await p.evaluate(()=>selectSection('stocks'));await p.waitForTimeout(30);assert.equal(await p.locator('#quotePrice').innerText(),'—');assert.equal(await p.locator('#sectionSave').isEnabled(),true);await p.close();});

test('draft and failed save retain actual quote/saved sidebar; discard restores controls',{timeout:20000},async()=>{const p=await open({...fixture,navigation:true,language:'en',width:390,configSaveFails:true});await p.evaluate(()=>selectSection('stocks'));await p.waitForTimeout(20);await p.locator('#stockSymbol0').fill('SCB');await p.locator('#stocksRange').selectOption('5d');assert.equal(await p.locator('#quoteSymbol').innerText(),'AAPL');assert.match(await p.locator('#quoteStatus').innerText(),/1d/);assert.match(await p.locator('#sectionState').innerText(),/yahoo.*1d/);await p.screenshot({path:path.join(__dirname,'../.worktrees/fidelity-fix/admin-fixture-stocks-draft.png'),fullPage:true});await p.locator('#sectionSave').click();await p.waitForFunction(()=>configPosts.length===1&&!saving);assert.match(await p.locator('#status').innerText(),/save|busy|retry/i);assert.equal(await p.locator('#stockSymbol0').inputValue(),'SCB');assert.equal(await p.locator('#quoteSymbol').innerText(),'AAPL');await p.screenshot({path:path.join(__dirname,'../.worktrees/fidelity-fix/admin-fixture-stocks-error.png'),fullPage:true});await p.locator('#sectionDiscard').click();assert.equal(await p.locator('#stockSymbol0').inputValue(),'PTT');assert.equal(await p.locator('#stocksRange').inputValue(),'1d');await p.close();});

test('actual weather flags select day night rain neutral and missing states; firmware file can be removed',{timeout:20000},async()=>{
 const p=await open({...fixture,navigation:true,language:'en',width:390});await p.evaluate(()=>selectSection('weather'));
 for(const [name,code,isDay,tile] of [['day',2,true,'1'],['night',2,false,'3'],['rain',61,null,'5'],['neutral-cloud',2,null,'4'],['missing',null,null,null]]){
  await p.evaluate(([weatherCode,isDay])=>{scenario.weather={hasData:weatherCode!==null,weatherCode,isDay,temperature:weatherCode===null?null:29.6,humidity:81};},[code,isDay]);await p.locator('#networkRefresh').click();await p.waitForFunction(()=>!networkLoading);
  if(tile!==null){assert.equal(await p.locator('#weatherArtwork').getAttribute('data-tile'),tile);assert.equal(await p.locator('#weatherArtwork').isVisible(),true);}else{assert.equal(await p.locator('#weatherTemperature').innerText(),'—');assert.equal(await p.locator('#weatherArtwork').isVisible(),false);}
  await p.screenshot({path:path.join(__dirname,`../.worktrees/fidelity-fix/admin-fixture-weather-${name}.png`),fullPage:true});
 }
 await p.evaluate(()=>selectSection('firmware'));await p.locator('#firmwareFile').setInputFiles({name:'desk-duo-app.bin',mimeType:'application/octet-stream',buffer:Buffer.alloc(2048)});assert.equal(await p.locator('#firmwareRemove').isVisible(),true);await p.locator('#firmwareRemove').click();assert.equal(await p.locator('#firmwareFile').evaluate(n=>n.files.length),0);assert.equal(await p.locator('#firmwareRemove').isVisible(),false);await p.close();
});

test('Wi-Fi connection badge and saved SSID occupy separate non-overlapping lines in TH and EN',{timeout:15000},async()=>{for(const language of ['th','en']){const p=await open({...fixture,navigation:true,language,width:390});await p.evaluate(()=>selectSection('wifi'));await p.waitForFunction(()=>!networkLoading);const bounds=await p.evaluate(()=>{const state=document.getElementById('connectionState').getBoundingClientRect(),saved=document.getElementById('ssidStored').getBoundingClientRect();return {stateBottom:state.bottom,savedTop:saved.top,stateText:document.getElementById('connectionState').textContent,savedText:document.getElementById('ssidStored').textContent};});assert.ok(bounds.savedTop>=bounds.stateBottom+6,language+' connection badge must be separated from saved SSID');assert.ok(bounds.stateText.length>0);assert.match(bounds.savedText,/Desk-WiFi/);await p.close();}});

test('Chromium exposes localized distinct control names and decorative provider marks after ticker and language edits',{timeout:20000},async()=>{
 const p=await open({...fixture,navigation:true,width:390,language:'en'}),cdp=await p.context().newCDPSession(p);
 const ax=async id=>{const {result}=await cdp.send('Runtime.evaluate',{expression:'document.getElementById('+JSON.stringify(id)+')'});const {nodes}=await cdp.send('Accessibility.getPartialAXTree',{objectId:result.objectId,fetchRelatives:false});return nodes[0];};
 for(const language of ['en','th']){
  await p.locator('#header'+language.toUpperCase()).click();await p.evaluate(()=>{selectSection('display');for(const detail of document.querySelectorAll('details'))detail.open=true;});
  const expected=language==='en'?['Section','Base brightness','Auto brightness','Touch calibration']:['ส่วนการตั้งค่า','ความสว่างฐาน','ความสว่างอัตโนมัติ','ปรับเทียบสัมผัส'];
  for(const [i,id] of ['sectionSelect','brightnessPercent','brightnessAuto','touch'].entries())assert.equal((await ax(id)).name?.value,expected[i],id+' accessible name '+language);
  for(const id of ['language','codexUsageMode','stocksRange']){assert.equal(await p.locator('#'+id).getAttribute('tabindex'),'-1');assert.equal(await p.locator('#'+id).evaluate(n=>n.closest('label').getAttribute('aria-hidden')),'true');}
  await p.evaluate(()=>selectSection('usage'));for(const id of ['claudeIdentity','codexIdentity']){assert.equal((await ax(id)).ignored,true);assert.equal(await p.locator('#'+id).getAttribute('alt'),'');}
  await p.evaluate(()=>selectSection('stocks'));const names=[];for(let i=0;i<5;i++){const name=(await ax('stockEnabled'+i)).name?.value;assert.match(name,language==='en'?/^Show /:/^แสดง /);assert.match(name,new RegExp(String(i+1)));names.push(name);}assert.equal(new Set(names).size,5);assert.ok(names[0].includes(await p.locator('#stockMarket0').inputValue()));assert.ok(names[0].includes(await p.locator('#stockSymbol0').inputValue()));
  await p.locator('#stockSymbol0').fill('SCB');await p.locator('#stockMarket0').selectOption('US');assert.match((await ax('stockEnabled0')).name.value,/US.*SCB/);assert.equal(await p.locator('#stockSymbol0').inputValue(),'SCB');
 }
 assert.equal(await p.evaluate(()=>configPosts.length+posts.length),0);await cdp.detach();await p.close();
});
