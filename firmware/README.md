# Desk Duo firmware (CYD แนวตั้ง)

Source receiver และ build สำหรับ ESP32-2432S028R / ESP32-D0WD-V3 / flash 4 MiB / ไม่มี PSRAM จอ 240×320 ใช้ LVGL 9.2.2, ESP-IDF 5.5.3 ผ่าน espressif32 6.13.0 บอร์ดถูกถอดออกตลอดงานนี้: build และ native rendering ผ่าน แต่ยังไม่ได้ flash, ทดสอบจอ/สัมผัสจริง หรือเรียก endpoint จริงจากบอร์ด

## Build และตรวจสอบบน Mac

จาก root ของ repository:

```sh
rtk proxy python3 -m venv .venv
rtk proxy .venv/bin/python -m pip install -r requirements-build.txt
rtk proxy .venv/bin/python scripts/pio.py run -d firmware -e cyd
rtk proxy .venv/bin/python scripts/test_firmware.py
rtk proxy .venv/bin/python -m unittest discover -s tests -p 'test_*.py' -v
rtk proxy node --test tests/stocks_endpoint.test.cjs tests/portal.test.cjs
rtk proxy .venv/bin/python scripts/package_firmware.py
```

`scripts/pio.py` ใช้ system trust เฉพาะ process ของ build ไม่ปิด TLS verification ไม่เปลี่ยน Python หรือ CA ของระบบ ส่วน `.platformio/`, `firmware/.pio/`, `managed_components/` และ `artifacts/` ไม่ถูก commit; `dependencies.lock` pin component จริง

Package สร้าง `artifacts/desk-duo-cyd.bin`, `.sha256`, `.json`, `desk-duo-app.bin` และ notices ตรวจ offset จาก metadata ของ build, factory partition, ขนาด, SHA256 และเทียบ bytes ของทั้งสามส่วนจริง ไม่มีคำสั่งเข้าถึง serial ใน script นี้

## ตั้งค่าหลังติดตั้งบนบอร์ดในอนาคต

จอแรกแสดง SSID `DeskDuo-xxxxxx`, รหัส WPA2 แบบสุ่ม 12 ตัว, URL `http://192.168.4.1`, และ Usage bearer token แบบสุ่ม 32 hex เชื่อม AP แล้วเปิด URL ใช้ HTTP Basic username `admin` และรหัสเดียวกับ AP การเชื่อมต่อ portal เป็น HTTP ภายในเครือข่าย local; ใช้ AP ที่มีรหัสหรือ LAN ที่เชื่อถือได้

กดแล้วปล่อย BOOT เพื่อแสดง/ซ่อนหน้าตั้งค่าอีกครั้ง โดยไม่ล้างค่า จอแสดง LAN IP หลัง Wi-Fi ต่อสำเร็จ ใช้ IP นี้แทน host ใน bridge config หาก Mac อยู่บน LAN เดียวกัน; ถ้า Mac ต่อ AP ใช้ `192.168.4.1` ไม่มี mDNS hostname ใน firmware รุ่นนี้

ตั้ง Wi-Fi, URL อากาศ/หุ้น, headers แบบ JSON, field mappings, refresh 30–86400 วินาที, auto rotation 0/ปิด หรือ 5–300 วินาที และ token จาก portal ค่า SSID/password/token/URL/headers ที่เว้นว่างจะคงค่าเดิม การล้าง Wi-Fi หรือ URL ต้องเลือกช่องล้าง; headers `{}` คือล้างทั้งหมด GET settings แสดง URL ที่ตัด query และไม่แสดง SSID/password/token/headers จึงไม่ใช้ URL ที่เห็นนั้นเขียนกลับอัตโนมัติ

บันทึกผ่าน validation และ round-trip ครบทุก field ลง NVS blob ก่อนเปลี่ยน active config (partition 88 KiB รองรับค่าเก่าและค่าใหม่ระหว่าง commit) ถ้า flash save ล้มเหลวได้ HTTP503 และค่าเดิมยังทำงาน Restart ปกติเก็บค่าเดิม หาก NVS มีข้อมูลเสีย firmware หยุดแทนการล้าง secrets โดยอัตโนมัติ

## Usage จาก Mac

ใช้ [bridge guide](../README.md) ส่งเฉพาะ Claude/Codex ไม่ต้อง CodexBar CLI ตั้ง endpoint เป็น `http://<LAN-IP>/api/usage` และ token จากจอในไฟล์ private นอก repo ตัวอย่างทดสอบในอนาคตหลังติดตั้ง:

```sh
rtk proxy curl --fail-with-body --request POST \
  --header 'Content-Type: application/json' \
  --header 'Authorization: Bearer REPLACE_WITH_DEVICE_TOKEN' \
  --data-binary @examples/usage-response.json \
  http://192.168.4.1/api/usage
```

ตัวเลขใน examples เป็นข้อมูลจำลอง Bearer token ไม่มีสิทธิ์แก้ settings และ Basic admin ใช้ส่ง usage ไม่ได้

## อากาศและ Google Sheets

ESP32 อ่านสองแหล่งเองทีละรายการจาก network task แยกกับจอ ตั้ง stocks URL ของ Apps Script ที่ deploy ตาม [คู่มือ Sheets](../integrations/google-sheets/README.md) รวม query `accessToken` หากใช้; Apps Script มัก redirect ไป `script.googleusercontent.com` Firmware ตามได้ไม่เกิน 4 ครั้งภายในงบ 10 วินาทีเดียว ตัด configured headers ทั้งหมดเมื่อ origin เปลี่ยนและไม่ใส่กลับแม้ redirect กลับ origin เดิม ห้าม HTTPS → HTTP

ตัวอย่าง Open-Meteo URL สำหรับกรุงเทพฯ (ต้องตั้งเองใน portal; ไม่ได้เรียกในงานนี้):

```text
https://api.open-meteo.com/v1/forecast?latitude=13.7563&longitude=100.5018&current=temperature_2m,relative_humidity_2m,weather_code&hourly=temperature_2m,precipitation_probability&forecast_days=1&timezone=Asia%2FBangkok
```

Default field mappings อยู่ใน [ตัวอย่าง config](../examples/device-config.example.json) HTTP body จำกัด 16384 bytes; response รายวันใหญ่เกิน cap ต้องลดขอบเขตที่ endpoint (เช่น `forecast_hours=4` ตาม API) ปรับ mapping ได้ใน portal ถ้า JSON ต่างจาก Open-Meteo เวลา local ที่ไม่มี UTC offset จะคง unknown

HTTPS ใช้ certificate bundle และตรวจ hostname/SNI ต้องมี SNTP time ก่อนยิง HTTPS ไม่ปิด certificate validation เมื่อเวลาไม่พร้อม UI ใช้ Bangkok timezone, clock แสดง `--:--` เมื่อไม่รู้เวลา จอใช้ข้อมูลจริงหรือ `--`/unknown เท่านั้น อายุ `age` หมายถึงเวลาตั้งแต่รับสำเร็จบนบอร์ด ไม่ได้ยืนยันอายุ quote ใน Sheet; stale แปลว่ารอบล่าสุดล้มเหลวหรือเกินสองช่วง refresh ราคาที่ขาดไม่แทนด้วย 0

## หน้าจอและข้อจำกัดที่ต้องตรวจบนฮาร์ดแวร์

หน้าแรก Claude อยู่บน Codex อยู่ล่าง แต่ละตัวมี usage primary/secondary และ reset ของตนเอง ตัวละคร loop ตลอดโดยไม่แสดงสถานะ working/idle ของ agent หน้าอากาศมีค่าปัจจุบันและ 4 forecast slots หน้าหุ้นแสดงครั้งละ 3 รายการ ถ้ามี 4–5 รายการจะสลับกลุ่มทุก 6 วินาที ทั้งสองหน้ามี mini characters ปัดซ้าย/ขวาหรือแตะแท็บเปลี่ยนหน้า

LCD: SPI2 MOSI13 MISO12 CLK14 CS15 DC2 BL21, 40MHz, RGB565, partial buffer 240×24×2 bytes เพียงก้อนเดียว ไม่มี full-screen buffer ซ้ำ Touch XPT2046: SPI3 CLK25 MOSI32 MISO39 CS33 IRQ36, 1MHz; ใช้ median samples และ config `touch` สำหรับ swapAxes/invertX/invertY/minX/maxX/minY/maxY

ก่อนใช้งานจริงให้ตรวจมุมทั้ง 4 ของภาพ/สัมผัส, portrait MADCTL0x48, สีแดง/น้ำเงิน, contrast/inversion, BOOT reveal, การเลื่อนหน้าและ animation ระหว่าง slow source/timeout, low heap ระหว่าง HTTPS+portal, NVS save/restart, wrong Basic/Bearer, invalid body, source changes ระหว่าง request, SNTP unavailable, body เกิน cap และ redirect ข้าม origin ไม่มีผล smoke test จริงในงานนี้

ฟอนต์ไทย 12/14px และ shaping มาจาก tamaclaude, Latin fallback คือ Montserrat ตรวจ cmap จริงและเรนเดอร์ด้วย native LVGL แล้ว ข้อความนอก ASCII/Thai/degree ที่ไม่มี glyph แสดง `?` รายละเอียดและ notices อยู่ใน [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)

## สำรอง / upload ในอนาคตเท่านั้น

คำสั่งต่อไปนี้เป็นคู่มือสำหรับเมื่อเจ้าของพร้อมต่ออุปกรณ์ ยังไม่ได้รันในงานนี้ แทน `PORT` ด้วย serial port จริง และเริ่มที่ baud115200 ถ้าการสำรองเร็วกว่าเคยล้มเหลวให้ retry ที่115200 โดยไม่เขียน flash จนอ่านครบและตรวจขนาด/เก็บ hash ได้:

```sh
rtk proxy .venv/bin/python .platformio/packages/tool-esptoolpy/esptool.py --chip esp32 --port PORT --baud 115200 read_flash 0 0x400000 .device-backups/original-4mb.bin
rtk proxy shasum -a 256 .device-backups/original-4mb.bin
```

สร้าง directory `.device-backups` ก่อนสำรอง เก็บ backup ไว้นอก Git ภาพรวมเริ่ม offset0 สำหรับติดตั้งครั้งแรกจะเขียน padding ทับ NVS เดิม จึงต้องสำรองก่อน:

```sh
rtk proxy .venv/bin/python .platformio/packages/tool-esptoolpy/esptool.py --chip esp32 --port PORT --baud 115200 write_flash 0x0 artifacts/desk-duo-cyd.bin
```

สำหรับอัปเดต Desk Duo ที่ใช้ partition เดิมแล้ว ให้ใช้ application-only เพื่อรักษา NVS:

```sh
rtk proxy .venv/bin/python .platformio/packages/tool-esptoolpy/esptool.py --chip esp32 --port PORT --baud 115200 write_flash 0x20000 artifacts/desk-duo-app.bin
```
