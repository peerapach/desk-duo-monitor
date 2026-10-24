// Embed bounded thumbnails from the same approved sources as the device assets.
// No runtime image dependency or external requests are needed by the portal.
import fs from 'node:fs/promises';
import {weatherTileRegion,weatherArtworkSource,weatherArtworkPixels} from './weather_artwork.mjs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {fileURLToPath} from 'node:url';
const require=createRequire(import.meta.url);
const sharp=require(process.env.SHARP_PATH||path.join(path.dirname(process.execPath),'../node_modules/sharp'));
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const images={};
for(const [name,file,size,tile] of [
 ['claude','claude-approved.png',64],['codex','codex-logo.png',64],
 ...Array.from({length:6},(_,i)=>['weather'+i,'weather-atlas.png',80,i])
]){
 const selection=tile===undefined?null:weatherArtworkSource(tile);
 const source=selection?path.join(root,selection.path):path.join(root,'firmware/assets',file);
 const {data,info}=await sharp(source).ensureAlpha().raw().toBuffer({resolveWithObject:true});
 let region;
 if(selection)region=weatherTileRegion(data,info.width,info.height,selection.atlasTile);
 else {
  let left=info.width,right=-1,top=info.height,bottom=-1;
  for(let y=0;y<info.height;y++)for(let x=0;x<info.width;x++){
   if(data[(y*info.width+x)*4+3]<=1)continue;
   left=Math.min(left,x);right=Math.max(right,x);top=Math.min(top,y);bottom=Math.max(bottom,y);
  }
  if(right<left)throw Error('Empty approved artwork: '+name);
  region={left,top,width:right-left+1,height:bottom-top+1};
 }
 const artwork=selection?sharp(Buffer.from(weatherArtworkPixels(data,info.width,tile)),{raw:{width:info.width,height:info.height,channels:4}}):sharp(source);
 const png=await artwork.extract(region)
  .resize(size,size,{fit:'contain',background:'#000000',kernel:'lanczos3'})
  .flatten({background:'#000000'}).png({palette:true,colours:128,compressionLevel:9}).toBuffer();
 images[name]='data:image/png;base64,'+png.toString('base64');
 console.log(name+': '+size+'px, '+png.length+' bytes PNG');
}
const target=path.join(root,'firmware/main/assets.cpp');
let source=await fs.readFile(target,'utf8');
const start='/* PORTAL_IMAGES_START */',end='/* PORTAL_IMAGES_END */';
const replacement=start+'\nconst portalImages='+JSON.stringify(images)+';\n'+end;
if(source.includes(start))source=source.slice(0,source.indexOf(start))+replacement+source.slice(source.indexOf(end)+end.length);
else source=source.replace('let activeSection=',replacement+'\nlet activeSection=');
await fs.writeFile(target,source);
