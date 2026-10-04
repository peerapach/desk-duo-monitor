# Pocket Studio character artwork

`pocket-studio-characters.png` is a transparent raster source image generated with the built-in imagegen tool from the user-approved `artifacts/design-reference/pocket-studio-claude-corrected.png` concept. The left mascot preserves Claude's familiar block silhouette; the right mascot is an original ceramic terminal robot representing Codex. This source contains no device configuration or live data.

The firmware should consume small compiled flash assets rather than decoding this full-resolution source on the ESP32. The source is retained for reproducibility and future animation work.

## Reproduce the firmware assets

```sh
rtk proxy /Users/peerapach.v/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node scripts/compile_character_assets.mjs --preview-dir artifacts/ui-character-assets
```

Run from the repository root. With another Node installation, set `SHARP_PATH` to its installed `sharp` module directory. The compiler defaults to `../node_modules/sharp` relative to the Node executable. It splits the source at the midpoint, finds each half's visible alpha bounds, preserves aspect ratio with square black padding, and converts to little-endian RGB565. The extraction contains isolated alpha=1 noise; the bounds scan excludes alpha<=1 (one part in 255), retaining every visibly opaque character edge. The original source PNG is unchanged.

The generated `firmware/main/generated/character_pixels.inc` contains two 64×64 main images and two 32×32 mini images, totalling 20,480 bytes. `character_assets.cpp` exposes constant LVGL descriptors and pixels in flash. No PNG decoder or source-resolution buffer runs on the device. Optional previews are decoded from the exact quantized RGB565 bytes, rather than from the unquantized source.

## Generation prompt

Use case: background-extraction / production character assets. The input image is the approved reference for TWO mascots on a black Pocket Studio screen. Create a clean production asset image containing ONLY these two exact characters, isolated on a genuinely TRANSPARENT background. Arrange them side by side with generous separation and margin, Claude on the left and Codex on the right. Full bodies, nothing clipped. Remove all UI, typography, hardware frames, map, progress bars, numbers, icons, and ground shadows. Preserve the approved identity and rendering faithfully: Claude is the flat frontal orange PIXEL creature, a connected rectangular stepped body with short rectangular side arms, exactly TWO black SQUARE eyes and FOUR rectangular short legs separated by three notches. It is not a round animal, no separate head/neck, no ears/nose/muzzle, keep grid-aligned orthogonal edges. Subtle copper shading and pale upper-left edge highlight exactly like reference. Codex is the same charming ivory ceramic robot with a slightly tilted rounded-rectangle head, glossy black face reading '>_' in mint light, two little articulated arms, two stout feet, mint chest indicator, soft ceramic material highlights; no antenna. Preserve its pose, body proportions, character silhouette and pleasant expression. Make each character occupy about the same square-sized asset region, and preserve empty space between them so a firmware asset compiler can isolate and render them at 64x64. Do not invent accessories or text, no new aesthetic. This is an image extraction asset, not a new UI design.
