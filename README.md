# Desk Duo

จอคู่หูบนโต๊ะสำหรับ **Claude + Codex usage, อากาศ และหุ้นไทย/US** บน ESP32 จอแนวตั้ง 240×320 พื้นดำ พร้อม Web Admin ภาษาไทย/English และอัปเดต firmware ผ่าน Wi-Fi

Mac ส่ง usage จาก CodexBar hooks เท่านั้น ส่วน ESP32 ดึงอากาศและหุ้นจากแหล่งข้อมูลที่ตั้งไว้โดยตรง ไม่ต้องมี Raspberry Pi หรือ server กลาง

## ฟีเจอร์

- Claude / Codex พร้อมตัวละครเคลื่อนไหว เปอร์เซ็นต์ usage และเวลา reset แยกตามช่วง
- Codex เลือก 5h + Weekly หรือ Weekly อย่างเดียวผ่านเว็บ
- อากาศและพยากรณ์ พร้อมภาพกลางวัน กลางคืน และฝน
- หุ้นไทย/US จาก Yahoo Finance พร้อมกราฟเล็ก หรือ JSON endpoint / Google Sheets
- Web Admin: Display, AI usage, Weather, Stocks, Wi-Fi, Firmware
- TH/EN, ปรับความสว่างจากจอและเว็บ และตารางแสงกลางวัน/กลางคืนโดยใช้ค่าที่ตั้งเป็นฐาน
- OTA แบบ application-only พร้อม boot rollback และรักษาค่าตั้งค่าเดิม

## เริ่มใช้งาน

1. ใช้บอร์ด **ESP32-2432S028R (CYD)** จอ ILI9341 / flash 4 MiB / ไม่มี PSRAM
2. [Build และติดตั้ง firmware](firmware/README.md) ทาง USB ครั้งแรก
3. เชื่อม AP `DeskDuo-xxxxxx` เปิด `http://192.168.4.1` เพื่อตั้ง Wi-Fi และเก็บรหัส admin / usage token จากหน้า Setup
4. [ตั้ง CodexBar hooks](docs/codexbar.md) บน Mac เพื่อส่ง Claude และ Codex
5. ตั้งอากาศ หุ้น ภาษา และความสว่างจาก Web Admin ที่ IP ของบอร์ด

บอร์ดที่ตั้งค่าแล้วให้อัปเดต OTA ด้วย `desk-duo-app.bin` ดูรายละเอียด migration และการรักษา NVS ในคู่มือ firmware

## โครงสร้าง repository

| ที่อยู่ | หน้าที่ |
| --- | --- |
| `firmware/` | ESP-IDF / LVGL, Web Admin, ภาพและฟอนต์ |
| `bridge/` | Python sender สำหรับ CodexBar hooks |
| `integrations/google-sheets/` | Apps Script สำหรับ JSON endpoint ทางเลือก |
| `examples/` | Config และ response ตัวอย่าง ไม่มีค่าของบอร์ดจริง |
| `scripts/` | Build, package, migrate และสร้าง assets |
| `tests/` | Python, Node, portable C++ และตัวเรนเดอร์ LVGL |
| `docs/` | คู่มือ protocol, การพัฒนา และสถานะงาน |
| `docs/superpowers/` | แผน/แบบในอดีต ไม่ใช่สถานะปัจจุบัน |

## คู่มือ

- [CodexBar hooks](docs/codexbar.md)
- [หุ้นไทย/US และกราฟ](docs/stocks.md)
- [Protocol และ JSON fields](docs/protocol.md)
- [การพัฒนาและการทดสอบ](docs/development.md)
- [เตรียม push ขึ้น GitHub](docs/github.md)
- [สถานะปัจจุบันและงานถัดไป](docs/status.md)

เก็บ config ส่วนตัวนอก repo หรือใช้ชื่อ `*.local.json` ที่ถูก ignore แล้ว ไม่ใส่รหัส Wi-Fi, admin password หรือ usage token ใน source ตัวอย่าง

Generated assets ถูกเก็บพร้อม source จึง build firmware ได้โดยไม่ต้องสร้างภาพ/ฟอนต์ใหม่ ดู [asset pipeline](firmware/assets/README.md) และ [third-party notices](THIRD_PARTY_NOTICES.md) สำหรับที่มาและใบอนุญาตของส่วนประกอบ โปรเจกต์ยังไม่ได้เลือกใบอนุญาตสำหรับโค้ดที่เขียนเพิ่ม
