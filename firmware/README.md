# Desk Duo firmware (CYD แนวตั้ง)

หุ้นไทย/US และกราฟเล็ก: [คู่มือ Yahoo Finance](../docs/stocks.md)
ค่าที่บันทึกแบบ JSON endpoint เดิมยังโหลดได้โดยไม่เปลี่ยน provider เอง

Source receiver และ build สำหรับ Desk Duo Firmware 2.2.3 บน ESP32-2432S028R / ESP32-D0WD-V3 / flash 4 MiB / ไม่มี PSRAM จอ 240×320 ใช้ LVGL 9.2.2, ESP-IDF 5.5.3 ผ่าน espressif32 6.13.0 รองรับ OTA ภายในเครือข่ายด้วย Basic admin และ boot rollback; build/native tests ไม่ยืนยันการทำงานบนอุปกรณ์หรือความถูกต้องของ live source

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

ตรวจ UI ด้วย LVGL จริงบน Mac (ใช้ dependencies ที่ target build ดาวน์โหลดแล้ว; ไม่มีการต่อบอร์ด):

```sh
rtk proxy .platformio/packages/tool-cmake/bin/cmake -S tests/native_ui -B .platformio/native-ui-regression -DCMAKE_BUILD_TYPE=Debug
rtk proxy .platformio/packages/tool-cmake/bin/cmake --build .platformio/native-ui-regression -j 8
rtk proxy .platformio/packages/tool-cmake/bin/ctest --test-dir .platformio/native-ui-regression --output-on-failure
```

UI regression ใช้ runtime/GPIO จำลองและ `ui.cpp` จริง ตรวจปุ่มท้ายจอทั้งสี่, LAN IP, token ที่เปลี่ยนขณะเปิด setup, layout/scroll เมื่อความยาวเปลี่ยน, การซ่อน/เปิดใหม่, usage window ที่มีเศษนาที, reset ปี 2100, auto rotation ที่ข้าม Setup และหยุด 15 วินาทีหลังแตะ พร้อมสร้างภาพ PPM ใน build directory

`scripts/pio.py` ใช้ system trust เฉพาะ process ของ build ไม่ปิด TLS verification ไม่เปลี่ยน Python หรือ CA ของระบบ ส่วน `.platformio/`, `firmware/.pio/`, `managed_components/` และ `artifacts/` ไม่ถูก commit; `dependencies.lock` pin component จริง

Package สร้าง `artifacts/desk-duo-cyd.bin`, `.sha256`, `.json`, `desk-duo-app.bin` และ notices ตรวจ offset จาก metadata ของ build, dual OTA partition, ขนาด, SHA256, image checksum/identity และเทียบ bytes ของทั้งสี่ส่วนจริง ไม่มีคำสั่งเข้าถึง serial ใน script นี้

ทดสอบ portal กับ DOM จริงแบบ offline เพิ่มเติมได้เมื่อมี Playwright และ Chromium อยู่แล้ว (ไม่ติดตั้ง/ดาวน์โหลดจาก test): ตั้ง `NODE_PATH` ให้ชี้ runtime ที่มี `playwright` และถ้าต้องใช้ Chromium ที่มีอยู่คนละ revision ให้ตั้ง `PLAYWRIGHT_CHROMIUM_EXECUTABLE` เป็น absolute path แล้วรัน `rtk proxy node --test tests/portal_dom.cjs` Test ปิด network requests ทั้งหมดและใช้ fixture ภายในเครื่อง ภาพทั้งสองภาษาอยู่ที่ `.platformio/native-ui-regression/portal-language-{th,en}.png`

## Web Admin

Web Admin มีหกส่วน: **Display**, **AI usage**, **Weather**, **Stocks**, **Wi-Fi** และ **Firmware**. จอกว้างใช้ sidebar; มือถือใช้ section selector. เปลี่ยนภาษาไทย/English หรือเปลี่ยนส่วนแล้ว draft ยังอยู่ แต่ละส่วนมีปุ่มบันทึกและยกเลิกของตัวเอง และแสดง saved values จาก API แยกจากค่าที่แก้ค้างไว้ ข้อความสถานะและข้อผิดพลาดมาจากคำตอบจริงของอุปกรณ์

Display จัดการภาษา ความสว่างฐาน 5–100% และตาราง Auto brightness; เวลาเริ่มกลางวัน/กลางคืนใช้ clock บน ESP32 โดยกลางวันใช้ฐานเต็มและกลางคืนใช้ครึ่งหนึ่ง (ขั้นต่ำ 5%). AI usage เลือกแสดง Codex ทั้ง 5-hour กับ Weekly หรือ Weekly อย่างเดียว และแก้ usage token โดยไม่เปิดเผยค่าที่บันทึกไว้. Weather มีตัวแก้ Open-Meteo สำหรับชื่อและพิกัด พร้อม Advanced สำหรับ endpoint, headers และ mappings เดิม; Stocks คงตัวเลือก Yahoo/legacy และช่วง Today/5 days; Wi-Fi แสดง SSID ที่บันทึกและสถานะว่ามี secret โดยช่องรหัสว่างคงค่าปัจจุบัน. Firmware อ่าน version/capacity จาก API และแสดงชื่อกับขนาดไฟล์จริงให้ตรวจ ก่อนขั้น upload ที่ยังใช้ auth/validation เดิม

## ตั้งค่าหลังติดตั้งบนบอร์ดในอนาคต

จอแรกแสดง SSID `DeskDuo-xxxxxx`, รหัส WPA2 แบบสุ่ม 12 ตัว, URL `http://192.168.4.1`, และ Usage bearer token แบบสุ่ม 32 hex เชื่อม AP แล้วเปิด URL ใช้ HTTP Basic username `admin` และรหัสเดียวกับ AP การเชื่อมต่อ portal เป็น HTTP ภายในเครือข่าย local; ใช้ AP ที่มีรหัสหรือ LAN ที่เชื่อถือได้

กดแล้วปล่อย BOOT เพื่อแสดง/ซ่อนหน้าตั้งค่าอีกครั้ง โดยไม่ล้างค่า จอแสดง LAN IP หลัง Wi-Fi ต่อสำเร็จ ใช้ IP นี้แทน host ใน bridge config หาก Mac อยู่บน LAN เดียวกัน; ถ้า Mac ต่อ AP ใช้ `192.168.4.1` ไม่มี mDNS hostname ใน firmware รุ่นนี้

ตั้ง Wi-Fi, URL อากาศ/หุ้น, headers แบบ JSON, field mappings, refresh 30–86400 วินาที, auto rotation 0/ปิด หรือ 5–300 วินาที และ token จาก portal ค่า SSID/password/token/URL/headers ที่เว้นว่างจะคงค่าเดิม การล้าง Wi-Fi หรือ URL ต้องเลือกช่องล้าง; headers `{}` คือล้างทั้งหมด GET settings สำหรับ Basic admin แสดง SSID ที่บันทึก (ไม่ใช่หลักฐานว่า radio ต่อเครือข่ายนี้อยู่), URL ที่ตัด query/fragment และสถานะตั้งค่าแล้ว/ยังไม่ตั้งค่าของรหัส Wi-Fi, usage token และ headers โดยไม่เปิดเผยค่าลับ ช่องแทนที่ URL/รหัส/token/headers ยังคงว่าง และส่งเฉพาะค่าที่แก้ไขจาก snapshot ที่โหลดสำเร็จ; ไม่แก้ไขจะไม่ POST หากโหลดไม่สำเร็จจะแสดงไม่ทราบและปิดบันทึก หากบันทึกสำเร็จแต่โหลดใหม่ล้มเหลวจะแจ้งว่าบันทึกแล้วและต้องโหลดหน้าใหม่ก่อนแก้ต่อ metadata นี้อยู่เฉพาะ public response ไม่เปลี่ยน Config หรือ bytes ใน NVS ส่วน Codex display form ยังโหลด/บันทึกแยกกัน

เลือกภาษาไทย / English สำหรับทั้งจอและเว็บในส่วน Display ค่าเริ่มต้นคือภาษาไทยเมื่อยังไม่เคยบันทึกภาษา เปลี่ยนตัวเลือกแล้วเว็บแสดงภาษานั้นให้ตรวจได้ก่อนบันทึก พร้อมแสดงภาษาที่บันทึกอยู่ บันทึกสำเร็จแล้วจอเปลี่ยนหน้าเดิมทันทีและจำค่าหลัง reboot ชื่อสถานที่/SSID/ชื่อหุ้นและค่าที่ผู้ใช้กรอกไม่ถูกแปล

เลือกการแสดง usage ของ Codex ในส่วน AI usage: `5h + Weekly` (ค่าเริ่มต้น) หรือ `Weekly อย่างเดียว` บันทึกแล้วจอเปลี่ยนทันทีโดยไม่ reboot การบันทึกส่วนนี้ไม่เปลี่ยน Wi-Fi/แหล่งข้อมูล และยังกดบันทึกไม่ได้จนโหลดค่าปัจจุบันสำเร็จ

ความสว่างฐานปรับได้ 5–100% ค่าเริ่มต้น 100% ในเว็บ เลื่อน slider เพื่อเลือกค่าฐาน แล้วกด **บันทึกการแสดงผล / Save display settings** เพื่อส่งเฉพาะ field ที่เปลี่ยน ค่าฐานที่บันทึกไม่ถูกเขียนทับด้วยค่าที่คำนวณตามเวลา เว็บแสดง mode/ภาษา/ฐาน/Auto/เวลาเริ่มกลางวันและกลางคืนที่บันทึก แยกจาก target (ค่าที่ต้องการ) และ last applied (คำขอปรับแสงที่ฮาร์ดแวร์รับสำเร็จล่าสุด ไม่ใช่การวัดแสงจริง; null คือยังไม่ทราบ)

เปิด **Auto brightness / ความสว่างอัตโนมัติ** และเลือกเวลาเริ่มกลางวัน/กลางคืนแบบ `HH:MM` ในเว็บ เวลาต้องต่างกัน ค่าเริ่มต้น 06:00/18:00 ใช้นาฬิกาและเขตเวลา ICT-7 บน ESP32 ไม่ใช่นาฬิกาใน browser กลางวันใช้เต็มค่าฐาน กลางคืนใช้ครึ่งหนึ่งปัด .5 ขึ้นและต่ำสุด5 เช่นฐาน11ให้กลางคืน6 รองรับช่วงกลางวันข้ามเที่ยงคืน เมื่อ Auto เปิดแต่นาฬิกายังไม่พร้อมจะใช้ฐานเดิมพร้อมสถานะรอนาฬิกา แล้วปรับเองเมื่อนาฬิกาพร้อม การเปลี่ยนเวลา/ช่วงไม่มีการบันทึก NVS ค่าเริ่มต้นและการย้ายข้อมูลเก่าเป็น Auto ปิด เพื่อใช้ความสว่างคงที่แบบเดิม หากต้องการปรับเองให้ปิด Auto

ที่จอ **ตั้งค่า / Setup** slider ยังคงแก้ค่าฐานและ preview แสงตาม Auto/เวลาในขณะนั้น โดยยังไม่เขียน NVS ปล่อยนิ้วจึงบันทึกเฉพาะฐานครั้งเดียวภายใต้ Settings lease เดียวกันและประเมินเวลา/ตารางล่าสุดอีกครั้งก่อนบันทึก ถ้าปรับ preview หรือค่าที่ต้องใช้ตอนปล่อยไม่สำเร็จจะไม่บันทึกฐานใหม่ หากยกเลิก/ออกจาก Setup/กด BOOT/เปลี่ยนภาษา หรือบันทึกล้มเหลว ระบบคืนค่าที่คำนวณจากฐานและตารางล่าสุดที่บันทึกพร้อมเวลาปัจจุบัน จอแสดงฐาน/ค่าที่ใช้ล่าสุดและสถานะ Auto; ตั้งเวลาและเปิดปิด Auto ผ่านเว็บ

เว็บอ่านสถานะแบบ read-only ประมาณทุก15วินาทีเมื่อหน้าเปิดอยู่ โดยไม่เขียนค่าหรือทับช่องที่กำลังแก้ รวมถึงภาษาและชื่อสถานที่ หาก POST สำเร็จแต่ GET อ่านกลับไม่สำเร็จ จะแสดงค่าที่บันทึกว่าไม่ทราบและรอ GET ที่ใช้ได้ก่อนบันทึกอีกครั้ง ส่วน POST ล้มเหลวยังคงแสดง snapshot ที่รู้ก่อนหน้า การอ่านที่เริ่มก่อน POST ไม่สามารถนำค่าเก่ากลับมาอ้างเป็นปัจจุบันได้

`GET /api/display` และ `POST /api/display` ใช้ Basic admin เท่านั้น GET คืน saved fields `codexUsageMode`, `language`, `brightnessPercent`, `brightnessAuto`, `brightnessDayStart`, `brightnessNightStart` และ read-only `brightnessTargetPercent`, `brightnessEffectivePercent`, `brightnessPhase` (`manual`, `day`, `night`, `clock_wait`) POST รับ JSON ไม่เกิน512bytes เช่น `{"brightnessPercent":50}`, `{"brightnessAuto":true}`, `{"brightnessDayStart":"07:15","brightnessNightStart":"19:45"}` หรือหลาย saved fields ในคำขอเดียว ฐานต้องเป็นจำนวนเต็ม5–100, Auto เป็น boolean และเวลาเป็น ASCII `HH:MM` ที่ไม่ซ้ำหลัง merge แม้ Auto ปิด ค่าผิด/type/key อื่น/read-only fields/คีย์ซ้ำ/คำขอว่างถูกปฏิเสธ ส่งเฉพาะ field ที่เปลี่ยนจะคง field อื่นไว้ Settings admission เดียวกันปิดการบันทึกทั้งเว็บและจอระหว่าง OTA (HTTP503); Origin ที่ส่งมาต้องตรงกับ HTTP Host ของเครื่อง

ค่าการแสดงผลเก็บแยกใน NVS namespace `deskduo`, scalar u32 key `ui_pref_v3`: mode bit0, ภาษา bit1, ฐาน bits2–8, Auto bit9, นาทีเริ่มกลางวัน bits10–20 และกลางคืน bits21–31 หาก v3 ไม่มี/typeผิด/ไม่ถูกต้อง จะ fallback v2 ที่ถูกต้อง (คงmode/ภาษา/ฐาน พร้อม Autoปิด/06:00/18:00) แล้ว `ui_pref`, `codex_layout`, defaults โดยไม่เขียนระหว่าง migration ค่าเก่าและ Config blob ไม่เปลี่ยน Single scalar บันทึกครบหก field: ESP-IDF ที่ pin ไว้เขียน NVS ตอน set ส่วน commit ยังเป็น no-op หลัง set/commit สำเร็จจึงเผยแพร่ preference พร้อมกัน ถ้า commit คืน failure จะคง active preference เดิม แม้ set อาจบันทึก scalar ใหม่ครบแล้ว

โหมดทั้งสองแสดง duration/reset ตามข้อมูลจริง และเรียกช่วง Codex 10080 นาทีว่า Weekly โหมด weekly เลือกช่วง 10080 นาทีจาก primary ก่อน secondary ถ้าไม่พบจึงยอมรับ secondary ที่ไม่มี duration หรือ primary ที่ไม่มี duration และเป็นช่วงเดียวตามการเลือกของผู้ใช้ ช่วงที่มี duration อื่นไม่ถูกเปลี่ยนชื่อเป็น weekly; ถ้าไม่มีช่วงที่เลือกได้แสดง unknown/-- พร้อมซ่อนค่า/reset/bar รองทั้งหมด ข้อมูล telemetry ไม่เปลี่ยน Claude แสดง duration ตามเดิม Weather และ Stocks ไม่มี mini sprites; Stocks แสดงกราฟเล็ก4แถวพร้อมเปลี่ยนช่วงวันนี้/5วัน

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

## Weather และ source configuration

ESP32 อ่านสองแหล่งเองทีละรายการจาก network task แยกกับจอ ตั้ง stocks URL ของ Apps Script ที่ deploy ตาม [คู่มือ Sheets](../integrations/google-sheets/README.md) รวม query `accessToken` หากใช้; Apps Script มัก redirect ไป `script.googleusercontent.com` Firmware ตามได้ไม่เกิน 4 ครั้งภายในงบ 10 วินาทีเดียว ตัด configured headers ทั้งหมดเมื่อ origin เปลี่ยนและไม่ใส่กลับแม้ redirect กลับ origin เดิม ห้าม HTTPS → HTTP

Weather editor เลือก Open-Meteo พร้อมชื่อสถานที่และพิกัด แล้วสร้าง endpoint ให้เมื่อผู้ใช้แก้ค่า provider/พิกัดโดยชัดเจน URL, headers และ mappings แบบกำหนดเองอยู่ใน Advanced และคงเดิมระหว่างโหลด/แก้ส่วนอื่น ตัวอย่าง endpoint สำหรับกรุงเทพฯ:

```text
https://api.open-meteo.com/v1/forecast?latitude=13.7563&longitude=100.5018&current=temperature_2m,relative_humidity_2m,weather_code&hourly=temperature_2m,precipitation_probability&forecast_hours=4&timezone=Asia%2FBangkok
```

ตาม [Open-Meteo API](https://open-meteo.com/en/docs) `forecast_hours=4` เริ่มที่ชั่วโมงปัจจุบันและคืน 4 ช่วงเวลา เช่น เวลา 12:45 ได้ 12:00–15:00; `forecast_days=1` พร้อม timezone เริ่มที่เที่ยงคืน จึงไม่เหมาะกับตัวอย่างนี้ ดู [response จำลอง](../examples/weather-response.json). UI แสดงค่าสภาพปัจจุบันและสามช่วงถัดไปจาก response; hourly samples ที่รู้เวลาและอยู่ก่อน observation `updatedAt` จะถูกข้าม จึงไม่แสดงชั่วโมงก่อนเวลาที่ source บอกว่าอัปเดต

Default field mappings อยู่ใน [ตัวอย่าง config](../examples/device-config.example.json): `current.is_day`, `hourly.weather_code` และ `hourly.is_day` เป็น optional. หาก source ไม่ส่งค่าเหล่านี้ เงื่อนไขหรือช่วงกลางวัน/กลางคืนจะคง unknown และไม่มีการเดาดวงจันทร์แทนข้อมูลที่หายไป เวลา observation `updatedAt` แปลงเป็น UTC ใน parser แล้วแสดงตาม source UTC offset; receipt age ใช้เวลาตั้งแต่บอร์ดรับสำเร็จและเป็นคนละค่า UI ไม่รายงานว่า observation สดเพียงเพราะเพิ่งรับ response. เวลา local ที่ไม่มี UTC offset คง unknown. HTTP body จำกัด 16384 bytes; ปรับ mappings ได้ใน portal

HTTPS ใช้ certificate bundle และตรวจ hostname/SNI ต้องมี SNTP time ก่อนยิง HTTPS ไม่ปิด certificate validation เมื่อเวลาไม่พร้อม นาฬิกา UI และตารางความสว่างใช้ ICT-7; เวลา weather ใช้ source UTC offset. นาฬิกาแสดง `--:--` เมื่อไม่รู้เวลา UI ใช้ข้อมูลจริงหรือ `--`/unknown เท่านั้น. Source receipt age บอกเวลาตั้งแต่บอร์ดรับสำเร็จ ไม่ยืนยันอายุข้อมูล ณ ต้นทาง; stale แปลว่ารอบล่าสุดล้มเหลวหรือเกินสองช่วง refresh ราคาที่ขาดไม่แทนด้วย 0

## หน้าจอและข้อจำกัดที่ต้องตรวจบนฮาร์ดแวร์

Desk Duo ใช้พื้นหลังดำบนจอ 240×320: header 24px, content 232px, LAN IP ที่ y=256 และปุ่ม Duo/Weather/Stocks/Setup ขนาด 60×44 ที่ y=276. Duo แสดง usage จาก provider จริงตามช่วงเวลาที่ส่งมา; ช่วงที่ไม่มีอยู่ยัง unknown และไม่ถูกแทนด้วย 0. Weather ใช้ source observation time และ offset ของ source แยกจากเวลา receipt; หาก source เก่าจะแสดง stale พร้อมเวลา observation. เงื่อนไขและ day/night artwork แสดงเฉพาะเมื่อ mapping มีข้อมูลรองรับ. Stocks แสดงกราฟ sparkline พร้อมสลับ Today/5 days จาก quote data ที่ได้รับ และใช้ timestamp/stale state จาก source. Setup ให้ปรับความสว่างฐานและตั้งค่าภาษา/ตารางตาม API โดยไม่อ้างว่า target brightness คือค่าที่วัดจากแสงจริง

ตัวละครและ weather artwork มาจาก [approved source PNGs และ asset pipeline](assets/README.md). Device ใช้ RGB565 images ที่ compile ไว้ใน flash และไม่ decode source PNG ขนาดเต็มขณะทำงาน; portal ฝัง PNG thumbnails เป็น data URL เพื่อทำงาน offline. ฟอนต์ Montserrat และ Thai fallback ใช้ร่วมกันใน target/native preview

LCD: SPI2 MOSI13 MISO12 CLK14 CS15 DC2 BL21, 40MHz, RGB565, partial buffer 240×24×2 bytes เพียงก้อนเดียว ไม่มี full-screen buffer ซ้ำ Touch XPT2046: SPI3 CLK25 MOSI32 MISO39 CS33 IRQ36, 1MHz; ใช้ median samples และ config `touch` สำหรับ swapAxes/invertX/invertY/minX/maxX/minY/maxY

ก่อนใช้งานจริงให้ตรวจมุมทั้ง 4 ของภาพ/สัมผัส, portrait MADCTL0x48, สีแดง/น้ำเงิน, contrast/inversion, BOOT reveal, การเลื่อนหน้าและ animation ระหว่าง slow source/timeout, low heap ระหว่าง HTTPS+portal, NVS save/restart, wrong Basic/Bearer, invalid body, source changes ระหว่าง request, SNTP unavailable, body เกิน cap และ redirect ข้าม origin ไม่มีผล smoke test จริงในงานนี้

ฟอนต์ไทย 12/14px และ shaping มาจาก tamaclaude, Latin fallback คือ Montserrat ตรวจ cmap จริงและเรนเดอร์ด้วย native LVGL แล้ว ข้อความนอก ASCII/Thai/degree ที่ไม่มี glyph แสดง `?` รายละเอียดและ notices อยู่ใน [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)

## OTA และ USB migration ที่รักษาการตั้งค่า

Partition layout dual OTA (introduced in release 2.0.0 and retained in 2.2.0): NVS `0x9000`/`0x16000`, PHY `0x1f000`/`0x1000` (ตำแหน่งและขนาดเดิม), OTA data `0x20000`/`0x2000`, OTA 0 `0x30000`/`0x1e0000`, OTA 1 `0x210000`/`0x1e0000`. NVS schema ไม่เปลี่ยน Wi-Fi/admin/usage token/endpoint/touch calibration จึงคงอยู่

บอร์ดที่ใช้ factory layout เดิมต้อง migrate ทาง USB ครั้งเดียว โดยใช้ไฟล์แยกที่ตรวจสอบแล้ว **ห้ามเขียน merged `desk-duo-cyd.bin` ที่ 0x0 บนบอร์ดที่ตั้งค่าแล้ว** ภาพ merged สำหรับ initial install เท่านั้น และ padding จะล้างการตั้งค่าเดิม `scripts/package_firmware.py` สร้าง app-only, bootloader, partition table, erased OTA initialization, manifest และ merged image; ไม่เข้าถึง serial

เริ่มจาก dry run (ไม่เปิดอุปกรณ์แม้ระบุ `--port` หากยังไม่มี `--flash`):

```sh
rtk proxy .venv/bin/python scripts/package_firmware.py
rtk proxy .venv/bin/python scripts/migrate_ota.py
```

ทั่วไป tool จะตรวจ chip/flash จริง แล้วอ่าน backup 4 MiB ใหม่ลง timestamp directory permission0700/ไฟล์0600 พร้อม SHA256 และ recovery metadata ก่อนเขียน segments:

```sh
rtk proxy .venv/bin/python scripts/migrate_ota.py --port PORT --flash
```

หากตั้งใจข้ามการสร้าง backup ให้ใช้ `--skip-backup`; ในโหมดนี้ tool ไม่เรียก `read_flash` และไม่สร้าง backup:

```sh
rtk proxy .venv/bin/python scripts/migrate_ota.py --port PORT --flash --skip-backup
```

ทุกกรณีเขียนเฉพาะ bootloader `0x1000`, partitions `0x8000`, OTA data `0x20000` และ app `0x30000`; ตรวจ sector erase ranges ไม่ทับ NVS/PHY และให้ esptool verify หลังเขียน ไม่ใช้ erase_flash ไม่แก้ eFuse และไม่เปลี่ยน credentials

หลัง migration เปิด portal ด้วย Basic admin เดิม เลือก `desk-duo-app.bin` แล้วกด Upload firmware เอง ไม่ upload เมื่อเลือกไฟล์ทันที Progress100% หมายถึงส่งข้อมูลครบและยังรอ SDK ตรวจ image; หลังรับรองแล้วจึง restart และลอง reconnect ไม่เกิน30ครั้ง (แต่ละ fetch timeout2s, พัก2s) ถ้าไม่ยืนยัน slot ใหม่ให้ตรวจจอ/โหลดหน้าใหม่ ไม่รายงานสำเร็จล่วงหน้า

API: `GET /api/firmware` ต้องใช้ Basic admin และคืน version, capacity, busy, runningPartition และ bootState (ESP-IDF enum) ไม่มี secrets. `POST /api/firmware` เป็น application-only binary ต้องใช้ Basic admin, `Content-Type: application/octet-stream`, `X-DeskDuo-Update: 1`, fixed positive Content-Length ภายใน slot; bearer usage token ไม่มีสิทธิ์ Origin ถ้ามีต้องเท่ากับ `http://` + Host แบบตรงตัว ไม่มี permissive CORS. SDK ตรวจ ESP32 image, project `desk_duo`, full checksum และ verified image length ต้องเท่ากับจำนวน bytes ที่รับจริงก่อนสลับ boot slot นี่คือ admin-authorized firmware ไม่ใช่ publisher-signature validation

Worker แยกจาก HTTP server ใช้ buffer4KiB, idle timeout10s, total deadline120s. Server จำกัด4 client sockets และเปิด LRU reclamation สำหรับ idle sessions; ESP-IDF ไม่เลือก async upload ที่กำลังทำงานมา evict; usage/status และ LVGL ยังให้บริการได้ settings POST ตอบ503 ระหว่าง maintenance; station reconfiguration/reconnect ใช้ atomic admission เดียวกัน Polling ภายนอกพักระหว่าง OTA (request ที่เริ่มแล้วอาจจบตาม deadline เดิม) ไม่มี LVGL call ใน worker

Bootloader rollback เปิดใน defaults และ effective sdkconfig. Trial image จะถูกยืนยันหลัง runtime/display/input/UI/AP/portal/network-task initialization สำเร็จ และ UI service iteration แรกเท่านั้น ไม่รอ internet หรือ station association. Reset/crash ก่อนจุดนั้นจะกลับ slot ก่อนหน้า

### Recovery และทดสอบ rollback

ถ้ามี complete backup ให้ตรวจ SHA256 ตาม private recovery.json ก่อน restore layout เดิมที่0x0ด้วย esptool; การ restore full backup เป็น recovery ที่ตั้งใจเท่านั้น ไม่ใช่การ migrate. เมื่อไม่มี backup ให้ใช้ validated segmented migration ข้างบนเพื่อคืน known-good release ทาง USB โดยยังรักษา NVS/PHY. Application serial offset ใหม่คือ `0x30000` สำหรับ OTA0; การเขียน app อย่างเดียวไม่เปลี่ยน OTA boot selection จึงแนะนำ Wi-Fi OTA หรือ segmented recovery ที่ reset OTA data แล้ว

Controlled rollback probe แยกจาก release ทั้ง source/build/output ไม่เรียก NVS/config API และ reset ตัวเองก่อน boot confirmation:

```sh
rtk proxy .venv/bin/python scripts/pio.py run -d tests/rollback_probe -e rollback_probe
```

ไฟล์ทดสอบคือ `tests/rollback_probe/.pio/build/rollback_probe/firmware.bin` (project `desk_duo`, version `rollback-probe`). **อย่าแจกเป็น release อย่า flash probe ผ่าน USB** หลังเตรียม USB recovery และยืนยัน known-good app แล้วจึง upload probe ผ่าน authenticated OTA เพื่อทดลอง bootloader rollback; บันทึกว่า version/slot กลับเป็นค่าเดิมจริง ก่อนเรียก rollback ว่าผ่าน Hardware-only checks ยังต้องทดสอบ settings หลัง migration, successful upload/reconnect/usage hook, truncated upload ไม่เปลี่ยน slot และ rollback จริง


Administrator `GET /api/network` reads the latest bounded weather attempt and
current accepted weather status. Usage Bearer credentials cannot read it. The
portal's **อัปเดตสถานะ** button only reads this status; it does not trigger a
source fetch. A failed status read does not block saving settings. A committed settings save
invalidates prior status, including responses already in flight; read status
again to inspect the current settings.

Diagnostics live in RAM and reset on reboot. They contain generation and
monotonic attempt/next-due times, current station and clock readiness (epoch
threshold, not an SNTP synchronization claim), whitelisted outcomes/parser
reasons, observed numeric HTTP/TLS/socket errors, bounded byte/redirect counts,
and internal 8-bit heap measurements. HTTP and TLS observations describe the
latest attempted redirect hop; unavailable error/status values are `null`.
The accepted temperature/humidity/code/hourly count and age come from the
runtime weather snapshot. `attempt.currentGeneration` distinguishes old
attempt evidence from the current saved source configuration. While a fetch
is running, the published record reports running; terminal phase/error details
appear when that attempt finishes. No URL, query, redirect target, headers,
response payload, credentials or SDK log text is retained in diagnostics.
Transport deadlines, certificate verification, response caps and scheduling
are unchanged. These diagnostics support device investigation and do not
establish a root cause for an absent forecast by themselves.
