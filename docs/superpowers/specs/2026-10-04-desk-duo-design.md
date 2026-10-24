# ESP32 Desk Duo — แบบที่ตกลงร่วมกัน

วันที่: 4 ตุลาคม 2026

## เป้าหมายและหน้าจอ

จอแนวตั้งสำหรับ Claude และ Codex พร้อมหน้าอากาศและหุ้น เลือก layout แบบแบ่งพื้นที่: Claude อยู่ด้านบน Codex อยู่ด้านล่าง แต่ละตัวมีแถบโควตาช่วงหลัก/รายสัปดาห์และเวลา reset ของตัวเอง

ตัวละครทั้งสองเล่น animation ต่อเนื่อง ในรุ่นแรกไม่ตรวจว่า session กำลังทำงาน รอคำตอบ หรือ idle หน้าอากาศและหุ้นมีตัวละครทั้งสองแบบย่ออยู่ด้านล่าง

จอมีสามหน้า: Usage, Weather, Stocks ตั้งให้สลับอัตโนมัติได้ การเปลี่ยนหน้าด้วย touch ขึ้นกับว่าจอจริงรองรับ touch หรือไม่ ความละเอียด 240×320 ใน mockup เป็นตัวอย่างจากบอร์ด CYD ยังไม่ใช่ข้อมูลฮาร์ดแวร์ของผู้ใช้ที่ยืนยันแล้ว

ตัวอย่างที่ผู้ใช้เลือก:

- Layout: `local design preview (not distributed)`
- Weather: `local design preview (not distributed)`
- Stocks: `local design preview (not distributed)`

ตัวเลขใน mockup เป็นข้อมูลจำลองทั้งหมด

## การแบ่งหน้าที่

```text
Mac: CodexBar app
    └─ usage hooks → ตัวส่งข้อมูล → ESP32

ESP32
    ├─ รับและเก็บ usage ของ Claude / Codex แยกกัน
    ├─ GET weather endpoint ผ่าน Wi-Fi
    └─ GET stocks endpoint ของ Google Apps Script ผ่าน Wi-Fi

Google Sheets → Apps Script → stocks JSON endpoint
```

Mac ส่งเฉพาะข้อมูล Claude/Codex ไม่ดึงอากาศหรือหุ้น ไม่จำเป็นต้องติดตั้ง CodexBar CLI สำหรับเส้นทาง app hooks การรับ usage จาก Mac ต้องมีตัวส่งข้อมูลที่รับ JSON จาก stdin ของ hook แล้วส่งต่อไปบอร์ด

ESP32 ดึงข้อมูลอากาศและหุ้นตามรอบของตัวเอง จึงอัปเดตได้เมื่อ Mac หลับหากบอร์ดยังเชื่อมต่ออินเทอร์เน็ต Usage เก็บค่าล่าสุดและแสดงอายุของข้อมูล ไม่เปลี่ยนข้อมูลที่ไม่ได้รับให้กลายเป็นศูนย์

## Usage

ใช้ hook `usage_updated` โดยแยกข้อมูลตาม `provider` ค่าเปอร์เซ็นต์ใน hook เป็นสัดส่วน 0–1 ต้องแปลงเป็น 0–100 สำหรับหน้าจอ ใช้ข้อมูลหน้าต่างหลัก/รองและ cadence ที่ได้รับจริง ไม่สร้างช่วงโควตาที่ provider ไม่ได้ส่งมา

เอกสาร CodexBar main ระบุว่า hook นี้จำกัดหนึ่งครั้งต่อ 600 วินาทีต่อ provider/account เวอร์ชันแอปที่ติดตั้งยังต้องตรวจว่ามี Settings → Hooks และรองรับ event นี้

แหล่งอ้างอิง: https://github.com/steipete/CodexBar/blob/main/docs/configuration.md#external-event-hooks

## Weather

ใช้ Open-Meteo เป็นแหล่งตัวอย่าง ตั้ง endpoint และ refresh interval ได้ ค่าเริ่มต้นที่เสนอคือ 900 วินาที เมือง/พิกัดเป็นค่าตั้งของผู้ใช้ ไม่ถือว่า Bangkok ใน mockup เป็นสถานที่ที่ยืนยันแล้ว

รองรับการอ่าน JSON ตาม mapping สำหรับอุณหภูมิ ความชื้น weather code และเวลาของข้อมูล พยากรณ์รายชั่วโมงและโอกาสฝนต้องร้องขอข้อมูลที่เกี่ยวข้องเพิ่มจาก endpoint ไม่อนุมานจากอุณหภูมิหรือความชื้น

แหล่งอ้างอิง: https://open-meteo.com/en/docs

## Stocks: Google Sheets + Apps Script

Google Sheets เป็นตารางรายการหุ้นและราคา Apps Script อ่านเฉพาะแท็บและคอลัมน์ที่กำหนด แล้วตอบ JSON ที่ ESP32 อ่านได้ ผู้ใช้เปลี่ยนรายการหุ้นโดยเพิ่ม/ลบแถว ไม่ต้องแก้ firmware

การเติมราคาหุ้นไทยอัตโนมัติเป็นหน้าที่ของแหล่งข้อมูลที่นำมาเติม Sheet แยกจากการอ่าน Sheet ไม่ถือว่า `GOOGLEFINANCE` รองรับหุ้นไทยทุกตัว และไม่ถือว่าการอ่าน Sheet ทำให้ราคาสดขึ้นเอง

รูปแบบคอลัมน์ที่เสนอ:

| คอลัมน์ | ความหมาย |
|---|---|
| symbol | ชื่อย่อ เช่น PTT |
| label | ชื่อที่แสดง ถ้าไม่มีใช้ symbol |
| price | ราคาตัวเลข |
| changePercent | เปอร์เซ็นต์เปลี่ยนแปลง เช่น 1.56 หมายถึง +1.56% |
| currency | สกุลเงิน เช่น THB |
| quotedAt | เวลาของราคาจากแหล่งข้อมูล ถ้าไม่มีแสดงว่าไม่ทราบ |

JSON ที่เสนอสำหรับ endpoint:

```json
{
  "schemaVersion": 1,
  "generatedAt": "2026-10-04T03:00:00Z",
  "items": [
    {
      "symbol": "PTT",
      "label": "PTT",
      "price": 32.5,
      "changePercent": 1.56,
      "currency": "THB"
    }
  ]
}
```

ข้อมูลใน JSON ข้างต้นเป็นตัวอย่าง `generatedAt` คือเวลาที่ endpoint สร้างคำตอบ ไม่ใช่เวลาที่ราคาตลาดเปลี่ยน `quotedAt` เป็น optional หากมีจะใช้ UTC ISO 8601 เช่นเดียวกัน ค่า numeric ที่ขาดหายไม่เติมเป็นศูนย์

ESP32 ตั้ง URL ของ Apps Script และ refresh interval ได้ ค่าเริ่มต้นที่เสนอคือ 300 วินาที HTTP client ต้องตาม redirect ของ Google ContentService ได้ การตั้งสิทธิ์ endpoint และ authentication ต้องเลือกให้เข้ากับบัญชี Google ที่ใช้ก่อน deploy; ไม่มีการเปลี่ยนสิทธิ์หรือเผยแพร่ Sheet จากการยืนยันแบบนี้

แหล่งอ้างอิง:

- https://developers.google.com/apps-script/guides/content
- https://developers.google.com/apps-script/guides/web
- https://support.google.com/docs/answer/3093281?hl=en

## การตั้งค่าและสถานะข้อมูล

กำหนด Wi-Fi, endpoint URL, รอบ refresh และ field mapping ผ่านหน้าตั้งค่าของ ESP32 เพื่อไม่ต้องให้ Mac เป็นตัวจัดการแหล่งข้อมูลอื่น สำหรับ API ที่ต้องใช้ credential เก็บค่าที่บอร์ด ไม่แสดงค่าในหน้าข้อมูลหรือ log

เมื่อโหลดไม่สำเร็จ เก็บค่าล่าสุดและแสดงว่าเก่า ถ้าไม่เคยโหลดสำเร็จแสดง `--` Animation และการเปลี่ยนหน้าไม่รอ HTTP request แสดงเวลาอัปเดตของแต่ละแหล่งข้อมูลแยกกัน

## ขอบเขตรุ่นแรก

- สองตัวละครพร้อม animation, usage จาก CodexBar app hooks
- หน้าจอแนวตั้งตาม layout แบบแบ่งพื้นที่
- Weather endpoint และ Stocks ผ่าน Google Sheets / Apps Script
- ไม่เพิ่ม calendar, crypto หรือการติดตามสถานะการทำงานของ session
- ยังไม่ deploy Apps Script, ไม่แก้บัญชีหรือ config CodexBar และไม่ flash บอร์ดในขั้นออกแบบ

## สิ่งที่ต้องทราบก่อนลงมือส่วนที่เกี่ยวข้อง

1. รุ่น ESP32, รุ่นจอ, ความละเอียด และการต่อขาจริง เพื่อเลือก firmware และ driver โดยไม่สมมติว่าเป็น CYD
2. Google Sheets ที่จะใช้ ชื่อแท็บ และตัวอย่างข้อมูล เพื่อยืนยันรูปแบบคอลัมน์และการเข้าถึง endpoint
3. แหล่งที่เติมราคาหุ้นลง Sheet และมีเวลาของราคาให้หรือไม่
4. เวอร์ชัน CodexBar และ event hooks ที่แอปนั้นรองรับ

ไดเรกทอรีโปรเจกต์ยังว่าง ณ วันที่ออกแบบ ไม่มี firmware, dependency, test หรือ Git repository เดิมให้ต่อยอด การเลือกว่าจะนำส่วนใดของ TamaClaude มาใช้ต้องพิจารณาหลังยืนยันฮาร์ดแวร์

## ขั้นลงมือที่ไม่ขึ้นกับฮาร์ดแวร์

ผู้ใช้อนุมัติให้เริ่มทำเมื่อ 4 ตุลาคม 2026 โดยใช้ `work-with-luna` ขั้นแรกส่งมอบ source ของ Apps Script, ตัวส่งข้อมูลบน Mac, รูปแบบข้อมูลร่วม, ตัวอย่าง config และคู่มือตั้งค่า ทดสอบด้วยข้อมูลและตัวรับจำลอง ไม่ถือว่าเป็นการทดสอบบน ESP32 จริงหรือ deploy บัญชี Google แล้ว

กำหนด receiver contract เป็น HTTP `POST /api/usage` บน LAN, `Content-Type: application/json`, และ `Authorization: Bearer <device token>` ตัวส่งไม่ตาม redirect และไม่ส่งข้อมูลบัญชีไปจอ Payload ใช้ `schemaVersion: 1`, `type: "usage"`, `provider`, `updatedAt`, และหน้าต่าง `primary` / `secondary` ที่มีข้อมูล แต่ละหน้าต่างใช้ `usedPercent` ในช่วง 0–100 และ optional `windowMinutes` / `resetsAt` วันที่ normalized เป็น UTC ISO 8601; ตัวส่งต้องไม่สร้างหน้าต่างหรือค่าเปอร์เซ็นต์เมื่อ hook ไม่ส่งมา

Apps Script ใช้ Script Properties `SPREADSHEET_ID`, `SHEET_NAME`, `ACCESS_TOKEN` ตรวจ query parameter `accessToken` ก่อนอ่าน Sheet อ่านไม่เกิน 200 แถวข้อมูล และตอบได้ไม่เกิน 5 รายการเพื่อให้เหมาะกับบอร์ด ตารางต้องมี header `symbol`, `price`; คอลัมน์อื่นเป็น optional ใช้ `THB` เมื่อ currency ไม่มี และใช้ symbol เมื่อ label ไม่มี ค่าตัวเลขว่างให้ omit; ค่าที่ไม่ใช่ตัวเลขหรือไม่ finite ให้ตอบ `invalid_sheet` แทนแสดงราคาปลอม ข้อมูลหลังรายการที่ 5 ไม่เป็นส่วนหนึ่งของ response

คำตอบผิดพลาดของ Apps Script ใช้ JSON `{ "schemaVersion": 1, "error": { "code": "..." } }` ไม่พึ่ง HTTP status ที่ ContentService ตั้งเองไม่ได้ เก็บ Sheet เป็น private ได้และ publish เฉพาะ web app ที่ตรวจ token การตั้งสิทธิ์และ deploy จริงยังต้องทำกับบัญชีของผู้ใช้

## ฮาร์ดแวร์ที่ยืนยันแล้วและขั้น firmware

ผู้ใช้ยืนยันว่าใช้ ESP32-2432S028R (CYD) รุ่นเดียวกับ TamaClaude การอ่านผ่าน esptool ที่พอร์ต `/dev/cu.usbserial-2120` พบ ESP32-D0WD-V3 revision 3.1 และ flash 4 MB วันที่ 4 ตุลาคม 2026 ไม่มีการเขียน flash ผู้ใช้ถอดบอร์ดออกเพราะแบตใกล้หมด ให้ทำ source, native tests และ build ต่อในเครื่อง; การ upload และทดสอบจอจริงรอการเชื่อมต่อครั้งหน้า

การสำรอง flash ครั้งแรกไม่สำเร็จที่ baud 460800 (Invalid head of packet 0xFF) และไม่ได้สร้างไฟล์ backup ที่ใช้งานได้ ต้องสำรองใหม่ด้วย baud ที่ต่ำกว่าก่อนเขียน firmware บนบอร์ดในอนาคต

### ชุด build และ driver

- PlatformIO Core 6.2.0 ใน project-local Python environment, `platformio/espressif32@6.13.0`, ESP-IDF 5.5.3, board profile esp32dev, 4 MB flash, ไม่ใช้ PSRAM
- LVGL 9.2.2 สำหรับ partial rendering และ widget layout ใช้ buffer 240×24 RGB565 เพียงชุดเดียว (11,520 bytes) ไม่สร้าง framebuffer เต็มจอหลายชุด
- ใช้ LCD SPI2 ตาม TamaClaude: MOSI 13, MISO 12, SCLK 14, CS 15, DC 2, backlight 21, ไม่มีขา reset แยก, SPI clock 40 MHz, inversion off, RGB565
- ใช้ค่า MADCTL 0x48 เป็นค่าเริ่มต้น portrait 240×320 ตาม ILI9341/TFT_eSPI ยังต้องตรวจบนจอจริงเรื่องทิศ สี และ inversion
- touch XPT2046 ใช้ SPI3 แยก: CLK 25, MOSI 32, MISO 39, CS 33, IRQ 36; ไม่ใช้ touch บน bus ของ LCD ค่า transform แนวตั้งยังต้องตรวจทั้งสี่มุมบนจอจริง มี calibration config สำหรับแก้โดยไม่แก้ driver
- ใช้ code/font บางส่วนของ TamaClaude ได้โดยเก็บ MIT copyright/license notice และตรวจ notice ของ font ที่นำมาใช้

### การทำงานและข้อมูล

Display loop เล่นสองตัวละครต่อเนื่อง โดยหน้าหลักแบ่ง Claude ด้านบน / Codex ด้านล่าง หน้า Weather/Stocks เก็บตัวละครสองตัวแบบย่อ หน้าเปลี่ยนทุก 20 วินาทีโดย default และเปลี่ยนด้วย swipe ได้ เมื่อตั้ง autoRotateSeconds=0 จะค้างหน้าเดิม

แยก HTTP fetch ออกจาก display loop เป็น FreeRTOS network task มี timeout และขนาด response สูงสุด 16,384 bytes ทำ fetch ทีละแหล่ง ไม่ถือ model lock ระหว่าง network IO เมื่อข้อมูลผิดหรือโหลดไม่สำเร็จคงค่าล่าสุดและแสดง stale; ก่อนได้ข้อมูลครั้งแรกแสดง `--` การอ่านข้อความจาก JSON ต้องจำกัด nesting depth และตรวจ types/finite values

Usage รับ protocol ที่กำหนดไว้ `/api/usage` ด้วย bearer token และอัปเดตข้อมูล provider แบบ atomic Reject schema/provider/field type ที่ผิด ไม่ใช้ account identity Clock และ reset countdown ไม่แสดงเวลาสมมติเมื่อยังไม่ sync SNTP

Stocks อ่าน schemaVersion 1, items สูงสุด 5 รายการ ถ้ามีมากกว่า 3 รายการให้หมุนชุดที่แสดง เพื่อให้เห็นทุกรายการ ใช้ source quotedAt ถ้ามี; generatedAt เป็นเวลา endpoint สร้างคำตอบ ไม่ใช่เวลาราคาตลาด

Weather อ่าน JSON ตาม field mapping มีอุณหภูมิ ความชื้น weather code และ optional hourly time/temperature/rain arrays แสดง forecast สูงสุด 4 ช่วงเวลา URL ที่ยังไม่ได้ตั้งไม่เกิด request ไม่ถือ Bangkok ใน mockup เป็นตำแหน่งจริงของผู้ใช้ เวลาท้องถิ่นที่ไม่มี timezone ใช้ utc_offset_seconds จาก endpoint เมื่อมี; ถ้าไม่มีให้ถือเวลา source เป็น unknown แทนเดา UTC

### การตั้งค่าและการเข้าถึง

เปิด Wi-Fi AP สำหรับตั้งค่าด้วย SSID เฉพาะเครื่องและ password random 12 ตัวอักษร เก็บใน NVS และแสดงบนจอ setup ใช้ password นี้เป็น HTTP Basic admin credential (`admin`) ของ portal ซึ่งแยกจาก usage bearer token ค่าที่สุ่มด้วย device RNG ไม่มี token/password คงที่ใน firmware

หน้า portal ตั้ง Wi-Fi, weather/stock URL, headers, refresh interval, weather field mapping, auto rotation และ usage bearer token ได้ ค่าที่ไม่ส่งมาใน patch คงค่าเดิม GET settings ไม่คืน Wi-Fi password, bearer token, headers หรือ query string ที่อาจมี accessToken ให้แสดงเฉพาะสถานะว่าตั้งแล้วและ non-secret settings หลัง authenticate

URL รับเฉพาะ http/https ที่ไม่มี userinfo, ไม่แสดง full URL หรือ secrets ใน log กำหนดเพดาน URL 1,024 bytes, header names/values 64/256 bytes และสูงสุด 6 headers ต่อ source; settings body ไม่เกิน 8,192 bytes Refresh interval 30–86,400 วินาที (weather default 900, stocks default 300), auto rotation 0 หรือ 5–300 วินาที

HTTPS ตรวจ certificate ด้วย ESP-IDF certificate bundle และ hostname verification ต้อง sync เวลา SNTP ก่อน request รองรับ redirect สูงสุด 4 ครั้ง; เมื่อ origin เปลี่ยนห้ามส่ง configured headers ต่อ และห้าม downgrade HTTPS ไป HTTP ไม่ใช้ insecure TLS

การบันทึก config ต้อง validate ก่อนเขียน NVS และ apply แบบ atomic Source ที่ URL ว่างถือว่าปิด การตั้งค่าเพื่อใช้งานจริงทำภายหลังได้โดยไม่ compile ใหม่ ไม่มีการใช้ข้อมูล Wi-Fi/Sheet/token จริงในการ build หรือ tests

### หลักฐานและขอบเขตการตรวจ

- https://github.com/thaitop/tamaclaude/blob/main/docs/hardware.md
- https://github.com/thaitop/tamaclaude/blob/main/firmware/main/ct_lcd.c
- https://github.com/thaitop/tamaclaude/blob/main/firmware/main/ct_touch.c
- https://github.com/Bodmer/TFT_eSPI/blob/master/examples/Test%20and%20diagnostics/Colour_Test/Colour_Test.ino
- https://github.com/platformio/platform-espressif32/releases/tag/v6.13.0
- https://docs.espressif.com/projects/esp-idf/en/v5.5.3/esp32/api-reference/protocols/esp_crt_bundle.html

ตรวจ native model/parsers/config validation, compile target ESP32 และสร้าง image ในเครื่องได้ การตรวจ orientation/touch, minimum heap ระหว่าง HTTPS และ animation, restore backup, Wi-Fi จริง, live Apps Script/CodexBar และการ flash ไม่ถือว่าสำเร็จจนกว่าจะตรวจจริงหลังเชื่อมต่อและตั้งค่าข้อมูลของผู้ใช้
