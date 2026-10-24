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

The original Thai font/shaping integration was adapted from the tamaclaude revision above. Current generated Thai, UI and numeric subsets are rebuilt with `scripts/export_thai_fonts.py` from the bundled Sarabun Medium and SemiBold sources in `firmware/main/fonts/source/`, under the same Sarabun OFL. Generated C headers record each source SHA256. The exporter uses FreeType and HarfBuzz during authoring only.

Subsets include Thai at12/14/18px, shared Thai/ASCII UI roles at16/18px, and numeric roles at24/48px. The shaping variants cover U+F700–U+F71B and U+F720–U+F765 in addition to standard Thai ranges. LVGL Montserrat supplies other Latin text. Preserve both Sarabun and Montserrat license files when redistributing.

## Artwork

Character and weather source assets are included under `firmware/assets/`; their selection and conversion provenance is described in its README. The Claude and Codex identities are used to identify the corresponding services. This repository does not grant rights to third-party trademarks.

## Build dependencies

ESP-IDF 5.5.3 (Espressif, Apache-2.0) and its third-party components, including cJSON (MIT), are managed build dependencies. LVGL 9.2.2 is MIT-licensed. Their original notices remain in the pinned downloaded SDK/component source trees. LVGL's generated Montserrat fonts are derived from Montserrat under SIL OFL 1.1. See the LVGL dependency's `LICENCE.txt` and `docs/CHANGELOG.rst` / font source metadata and https://github.com/google/fonts/tree/main/ofl/montserrat for font licensing. The release artifact directory includes copies of this notice, Sarabun OFL, LVGL MIT, Montserrat OFL, ESP-IDF license, and cJSON MIT.
