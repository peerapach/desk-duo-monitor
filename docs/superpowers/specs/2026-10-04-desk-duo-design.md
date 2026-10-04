# ESP32 Desk Duo — แบบที่ตกลงร่วมกัน

วันที่: 4 ตุลาคม 2026

## เป้าหมายและหน้าจอ

จอแนวตั้งสำหรับ Claude และ Codex พร้อมหน้าอากาศและหุ้น เลือก layout แบบแบ่งพื้นที่: Claude อยู่ด้านบน Codex อยู่ด้านล่าง แต่ละตัวมีแถบโควตาช่วงหลัก/รายสัปดาห์และเวลา reset ของตัวเอง

ตัวละครทั้งสองเล่น animation ต่อเนื่อง ในรุ่นแรกไม่ตรวจว่า session กำลังทำงาน รอคำตอบ หรือ idle หน้าอากาศและหุ้นมีตัวละครทั้งสองแบบย่ออยู่ด้านล่าง

จอมีสามหน้า: Usage, Weather, Stocks ตั้งให้สลับอัตโนมัติได้ การเปลี่ยนหน้าด้วย touch ขึ้นกับว่าจอจริงรองรับ touch หรือไม่ ความละเอียด 240×320 ใน mockup เป็นตัวอย่างจากบอร์ด CYD ยังไม่ใช่ข้อมูลฮาร์ดแวร์ของผู้ใช้ที่ยืนยันแล้ว

ตัวอย่างที่ผู้ใช้เลือก:

- Layout: `/Users/peerapach.v/.codex/visualizations/2026/10/04/01a10497-c77a-7533-a410-77b7fc0e9aca/desk-duo-preview.html`
- Weather: `/Users/peerapach.v/.codex/visualizations/2026/10/04/01a10497-c77a-7533-a410-77b7fc0e9aca/desk-weather-preview.html`
- Stocks: `/Users/peerapach.v/.codex/visualizations/2026/10/04/01a10497-c77a-7533-a410-77b7fc0e9aca/desk-stocks-preview.html`

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
