# Desk Duo image sources and asset pipeline

The checked-in source images include the approved Desk Duo artwork and separately identified weather variants:

- `claude-approved.png` is the orange Claude character artwork selected for the device and portal.
- `codex-logo.png` is the exact white scalloped Codex cloud mark with the black `>_` glyph selected by the user. It is used as that logo; the project does not substitute a robot mascot.
- `weather-night-approved.png` preserves the moon/cloud pixels from the approved all-screen board `exec-77ccaed0-3df8-4dfd-be10-641f01738fa6.png`, crop `(594,64,158,147)`.
- `weather-day-approved.png` and `weather-rain-approved.png` preserve the large illustrations from the earlier approved weather triptych `exec-9fe8e6bf-096a-45a9-a205-ce1627fcbb6c.png`, crops `(319,90,296,305)` and `(1575,90,280,315)`. The later all-screen board supplies the night scene, while the triptych supplies day and rain scenes absent from that board.
- `weather-forecast-cloud-approved.png` and `weather-forecast-rain-approved.png` preserve the small forecast symbols from the all-screen board, crops `(460,299,42,30)` and `(574,299,33,35)`. Forecast symbols use these dedicated images rather than shrinking the detailed hero scenes.
- `weather-atlas.png` contains generated variants for the remaining conditions. Its row-major order is clear day, partly cloudy day, clear night, an intentionally empty partial-night tile, cloudy, and rain. The shared source selector overrides partial day, partial night and rain with the original approved images above; those atlas cells do not define the delivered hero artwork.

These are source artwork only; they contain no device readings or configuration. All assets are compiled locally and embedded in firmware. The target does not decode the source-resolution PNGs or fetch images from a CDN.

## Rebuild assets

Run these from the repository root after installing the Node runtime and `sharp` dependency used by the project:

```sh
rtk proxy node scripts/compile_character_assets.mjs --preview-dir .worktrees/unified-product/assets/characters
rtk proxy node scripts/compile_weather_assets.mjs --preview-dir .worktrees/unified-product/assets/weather
rtk proxy node scripts/compile_portal_assets.mjs
```

`SHARP_PATH` may point to the installed `sharp` module directory when the default module lookup does not find it. The compilers preserve aspect ratio, flatten against black, and convert device artwork to little-endian RGB565. `compile_character_assets.mjs` generates the two characters at 64×64 and 32×32 in `firmware/main/generated/character_pixels.inc`. `compile_weather_assets.mjs` generates six 96×96 hero images and six 28×28 forecast images in `firmware/main/generated/weather_pixels.inc`.

Both weather pipelines use `scripts/weather_artwork.mjs` for source selection, RGB-aware visible bounds and removal of the dark screenshot matte. The original source PNGs stay unchanged. Compiled pixels with peak RGB ≤20 become black, values 20–36 are feathered, and pixels ≥36 retain their original channel values before resizing and RGB565 conversion. The night image additionally feathers its first five columns to soften the clipped screenshot boundary; its remaining interior pixels are unchanged by that edge treatment. This avoids a rectangular screenshot backdrop while preserving the approved illuminated artwork.

The weather compiler also compiles four consistent curved navigation outlines from local SVG paths into active/inactive 24×22 RGB565 images. Device navigation keeps its 60×44 touch targets, separate captions and mint active marker. No runtime SVG dependency is required.

`compile_portal_assets.mjs` creates compact, palette PNGs and writes them as data URLs into the marked image constants in `firmware/main/assets.cpp`: 64px identity images and 80px weather images. The embedded portal stays offline. Its root route CSP permits `img-src data:` for those images while retaining the existing script, style, connect, frame, base, and form restrictions.

The optional preview directories belong under `.worktrees/` so they stay with this repository's scratch outputs. Device pixel previews are decoded from the quantized RGB565 bytes used by the firmware.
