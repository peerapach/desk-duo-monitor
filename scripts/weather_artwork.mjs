// RGB-aware bounds for the opaque approved 3×2 weather atlas. Shared by device and portal.
// Coordinates are absolute in the full RGBA source; a margin preserves the soft glow.
export function weatherTileRegion(data, width, height, tile) {
  if ((tile!==null && (!Number.isInteger(tile) || tile < 0 || tile > 5)) || data.length !== width * height * 4) throw new Error('Invalid weather atlas');
  const tw=tile===null?width:Math.floor(width/3), th=tile===null?height:Math.floor(height/2), x0=tile===null?0:(tile%3)*tw, y0=tile===null?0:Math.floor(tile/3)*th;
  let left=x0+tw, right=-1, top=y0+th, bottom=-1;
  for(let y=y0;y<y0+th;y++) for(let x=x0;x<x0+tw;x++) {
    const i=(y*width+x)*4;
    if(data[i+3]<=1 || Math.max(data[i],data[i+1],data[i+2])<=32) continue;
    left=Math.min(left,x);right=Math.max(right,x);top=Math.min(top,y);bottom=Math.max(bottom,y);
  }
  if(right<left) throw new Error('Empty weather artwork');
  const margin=Math.max(2,Math.round(Math.max(tw,th)*0.025));
  left=Math.max(x0,left-margin);right=Math.min(x0+tw-1,right+margin);
  top=Math.max(y0,top-margin);bottom=Math.min(y0+th-1,bottom+margin);
  return {left,top,width:right-left+1,height:bottom-top+1};
}

// Both compilers use the exact same approved production-source overrides.
// A null atlasTile tells weatherTileRegion to bound a standalone source image.
export function weatherArtworkSource(tile) {
  if(tile===1)return {path:'firmware/assets/weather-day-approved.png',atlasTile:null};
  if(tile===5)return {path:'firmware/assets/weather-rain-approved.png',atlasTile:null};
  return tile===3?{path:'firmware/assets/weather-night-approved.png',atlasTile:null}:{path:'firmware/assets/weather-atlas.png',atlasTile:tile};
}
export function weatherForecastSource(tile) {
  if(tile===1 || tile===3 || tile===4)return 'firmware/assets/weather-forecast-cloud-approved.png';
  if(tile===5)return 'firmware/assets/weather-forecast-rain-approved.png';
  return null;
}

// Remove only the dark screenshot matte. Approved bright cloud/moon pixels stay byte-identical.
export function weatherBlackMatte(data) {
  const result=Uint8Array.from(data);
  for(let i=0;i<result.length;i+=4) {
    const peak=Math.max(data[i],data[i+1],data[i+2]);
    const factor=peak<=20?0:peak>=36?1:(peak-20)/16;
    for(let c=0;c<3;c++)result[i+c]=Math.round(data[i+c]*factor);
  }
  return result;
}

// The approved night screenshot ends through the left cloud. Feather that boundary only.
export function weatherArtworkPixels(data,width,tile) {
  const result=weatherBlackMatte(data);
  if(tile!==3)return result;
  for(let i=0;i<result.length;i+=4) {
    const x=(i/4)%width;if(x>=5)continue;
    const t=x/5, factor=t*t*(3-2*t);
    for(let c=0;c<3;c++)result[i+c]=Math.round(result[i+c]*factor);
  }
  return result;
}
