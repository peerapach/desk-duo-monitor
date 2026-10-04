# Third-party notices

Desk Duo reuses the following sources. Retain this file and the font license when distributing firmware source or binaries.

## tamaclaude program sources

Source: https://github.com/thaitop/tamaclaude/tree/2da01c07e4f42aa42251793c06c93495a8973f30

The ILI9341 register initialization in `firmware/main/display.cpp`, the Thai shaping algorithm/table adaptation in `firmware/main/assets.hpp`, and generated Thai fonts originate from that revision. The font data is additionally governed by the Sarabun OFL below. Driver pin values follow this board's discovery notes; portrait MADCTL is changed to 0x48.

MIT License

Copyright (c) 2026 Uthai Moolpak

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Sarabun font data

Copyright 2018 The Sarabun Project Authors (https://github.com/cadsondemak/Sarabun).
SIL Open Font License 1.1; full license: [Sarabun-OFL.txt](firmware/main/fonts/Sarabun-OFL.txt).

Generated/subset font names are `ct_font_thai_12` and `ct_font_thai_14`; their original generated C sources are preserved, with no conversion edits. Provenance is the tamaclaude revision above (the generator does not record the upstream Sarabun Git revision). The actual bundled Sarabun-Regular.ttf input has SHA256 `226d4f368fbc0457990ddef2692679badfd2c1a4e89e5ac4d43c10ba7743b2f1`. Font source and generation references at that same revision: `tools/fonts/Sarabun-Regular.ttf`, `tools/export_thai_font.py`, `tools/thai.toml`, `tools/gen/thai.py`, and `tools/gen/thai_table.py`.

The generated cmaps cover U+0E01–U+0E3A, U+0E3F–U+0E5B, and U+F700–U+F719. Montserrat supplies ASCII and degree sign; unsupported display characters are rendered as `?`. The shaper maps upper/lower/tone marks onto the supplied variants and normalizes already-shaped input. Composite line metrics preserve marks above and below Latin text.

## Build dependencies

ESP-IDF 5.5.3 (Espressif, Apache-2.0) and its third-party components, including cJSON (MIT), are managed build dependencies. LVGL 9.2.2 is MIT-licensed. Their original notices remain in the pinned downloaded SDK/component source trees. LVGL's generated Montserrat fonts are derived from Montserrat under SIL OFL 1.1. See the LVGL dependency's `LICENCE.txt` and `docs/CHANGELOG.rst` / font source metadata and https://github.com/google/fonts/tree/main/ofl/montserrat for font licensing. The release artifact directory includes copies of this notice, Sarabun OFL, LVGL MIT, Montserrat OFL, ESP-IDF license, and cJSON MIT.
