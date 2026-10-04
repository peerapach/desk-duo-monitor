# ESP32 Desk Duo data feeds

โปรเจกต์นี้มี source สำหรับ endpoint หุ้นจาก Google Sheets และตัวส่ง usage จาก CodexBar app บน Mac ไปยัง ESP32 ขณะนี้ยังไม่มีการ deploy Apps Script หรือ firmware receiver; เครื่องรับ `POST /api/usage` จะทำในงาน firmware ถัดไป จึงยังไม่มีการทดสอบส่งข้อมูลเข้าอุปกรณ์จริง

## หุ้นจาก Google Sheets

บอร์ดจะอ่าน JSON จาก Apps Script ส่วน Mac ไม่อ่าน Sheet ให้ดู [คู่มือตั้งค่า Apps Script และ Google Sheets](integrations/google-sheets/README.md) และ [ตัวอย่าง response](examples/stocks-response.json) ค่าหุ้นในตัวอย่างเป็นข้อมูลจำลอง

## Usage จาก CodexBar app

CodexBar ส่ง event `usage_updated` เป็น JSON ทาง stdin ให้โปรแกรมที่ตั้งไว้ใน Settings → Hooks ตัวส่งนี้รันตรงจาก absolute executable และ arguments ของ hook โดยไม่ใช้ shell และไม่ต้องติดตั้ง CodexBar CLI เอกสาร CodexBar ระบุว่ามีช่วงจำกัดการเรียกซ้ำ 600 วินาทีต่อ provider/account/window สำหรับ event นี้ รายละเอียดอาจเปลี่ยนตามเวอร์ชัน ให้ตรวจ Settings → Hooks และเอกสาร [External event hooks](https://github.com/steipete/CodexBar/blob/main/docs/configuration.md#external-event-hooks) ของเวอร์ชันที่ติดตั้ง

event ใช้เปอร์เซ็นต์แบบสัดส่วน `0–1`; sender แปลงเป็น `usedPercent` ช่วง `0–100` เวลา `updatedAt` มาจาก `timestamp` ใน event และแปลงเป็น UTC ไม่ได้ใช้เวลาที่ส่ง request แทน ข้อมูล `primary` และ `secondary` จะมีเฉพาะเมื่อ event มีเปอร์เซ็นต์ของหน้าต่างนั้น ค่า `0` เป็นค่าจริง ส่วนหน้าต่างที่ขาดจะไม่ถูกสร้างขึ้นใหม่

ตรวจตัวอย่างโดยไม่ใช้ network:

```sh
rtk proxy python3 bridge/codexbar_sender.py --dry-run < examples/usage-event.json
```

ผลลัพธ์ที่ normalize แล้วตรงกับ [usage response fixture](examples/usage-response.json) เมื่อเทียบเป็น JSON ตัวอย่าง event และ response เป็นข้อมูลจำลอง

### เก็บ config ส่วนตัว

คัดลอก `examples/bridge-config.example.json` ไปไว้ข้างนอก repository เช่น `~/.config/desk-duo/bridge.local.json` ตั้ง permission ของ directory เป็น `700` และไฟล์เป็น `600` แล้วแทน endpoint กับ token ตัวอย่างด้วยค่าที่ตั้งใน firmware จริง:

```sh
rtk proxy install -d -m 700 ~/.config/desk-duo
rtk proxy install -m 600 examples/bridge-config.example.json ~/.config/desk-duo/bridge.local.json
```

อย่า commit ไฟล์ `bridge.local.json` หรือใส่ token จริงใน hook arguments, source, หรือ log ตัวอย่างนี้ยังไม่ใช่ค่า config สำหรับบอร์ดที่ deploy แล้ว

ใน Settings → Hooks ของ CodexBar ให้นำเฉพาะ object `hooks` จาก [ตัวอย่าง hook merge fragment](examples/codexbar-hooks.example.json) ไปรวมกับ config เดิม แล้วแทน placeholder ด้วย absolute path ของ Python, script และ `bridge.local.json` คง `version`, `providers` และการตั้งค่าของ CodexBar เดิมไว้ ตัวอย่างนี้มี rule ที่เปิดใช้แยกสำหรับ Claude และ Codex แต่ไม่ได้แก้ config บนเครื่องให้

CodexBar เรียก executable โดยตรง ไม่ผ่าน shell; `arguments` เป็นรายการ argument แยกกัน และ JSON event มาทาง stdin ตามรูปแบบของ app ตัวส่งจะใช้ HTTP `POST /api/usage`, ส่ง `Content-Type: application/json` กับ `Authorization: Bearer …`, ปฏิเสธ redirect และถือว่า status 2xx คือส่งสำเร็จ การส่งล้มเหลวไม่มี retry อัตโนมัติ รายละเอียด field อยู่ใน [protocol](docs/protocol.md)

หลังมี firmware receiver แล้ว สามารถทดสอบ POST ด้วยตัวอย่างที่ normalize แล้ว (แทน host และ token ด้วยค่าจริงจากการตั้งค่าของคุณ):

```sh
rtk proxy curl --fail-with-body --request POST \
  --header 'Content-Type: application/json' \
  --header 'Authorization: Bearer REPLACE_WITH_DEVICE_TOKEN' \
  --data-binary @examples/usage-response.json \
  http://esp32.local/api/usage
```

sender และ fixtures ไม่ fetch weather หรือหุ้นบน Mac อากาศใช้แหล่งที่ ESP32 ตั้งค่าเอง เช่น Open-Meteo; การตั้งค่าใน firmware เป็นงานถัดไปพร้อมตรวจ display variant และ pin mapping ของบอร์ด ตัว chip ที่ตรวจพบคือ ESP32-D0WD-V3 rev 3.1 พร้อม 4 MiB flash; ข้อมูลนี้ยังไม่ยืนยันทุก revision ของ CYD หรือการต่อขาจอจริง
