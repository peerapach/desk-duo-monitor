# Development

รันคำสั่งจาก repository root ไม่ต้องติดตั้ง RTK หรือใช้ Codex เพื่อ build

## Build firmware

ใช้ Python 3.9+, Git และ toolchain ที่ PlatformIO จัดการให้:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements-build.txt
.venv/bin/python scripts/pio.py run -d firmware -e cyd
.venv/bin/python scripts/package_firmware.py
```

`scripts/pio.py` ใช้ cache `.platformio/` และ system trust store เฉพาะ build process ส่วน dependency versions อยู่ใน `firmware/platformio.ini`, `firmware/main/idf_component.yml` และ `firmware/dependencies.lock`

ผล package อยู่ใน `artifacts/` ซึ่งไม่เข้า Git แจก binary ผ่าน GitHub Releases ภายหลังได้ ดู offset, USB migration และ OTA ใน [firmware guide](../firmware/README.md)

## Tests

Python sender/tools และ Node portal/endpoint:

```sh
python3 -m unittest discover -s tests -p 'test_*.py' -v
node --test tests/stocks_endpoint.test.cjs tests/portal.test.cjs
```

Portable C++ / SDK stubs ต้องมี C/C++ compiler และ SDK cJSON ที่ build ด้านบนดาวน์โหลดไว้:

```sh
.venv/bin/python scripts/test_firmware.py
```

Native UI ใช้ `ui.cpp` กับ LVGL จริง จำลอง runtime/GPIO และสร้างภาพ PPM โดยไม่ต้องต่อบอร์ด:

```sh
cmake -S tests/native_ui -B .platformio/native-ui
cmake --build .platformio/native-ui --parallel
ctest --test-dir .platformio/native-ui --output-on-failure
```

## Assets

Generated C fonts และ RGB565 arrays เป็น source ที่ต้อง commit พร้อม PNG/TTF ต้นฉบับ ดู [asset pipeline](../firmware/assets/README.md) หากต้องการสร้างภาพใหม่ ส่วน font exporter ใช้ Pillow, fontTools และ `hb-shape` (HarfBuzz) สำหรับ authoring เท่านั้น ไม่อยู่ใน runtime ของ ESP32

## Working files

เก็บงานทดลอง ภาพเปรียบเทียบ และรายงานเฉพาะเครื่องใน `.worktrees/` ไม่ใช้ temp directory นอก repo ไม่ commit `.worktrees/`, `.platformio/`, `.venv/`, `.device-backups/`, `artifacts/`, `node_modules/` หรือ `*.local.json` ส่วน `graft/` เป็น checkout อื่น ไม่ใช่ส่วนของ Desk Duo
