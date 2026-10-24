# Current status

## Firmware 2.2.3

รุ่นล่าสุดที่ติดตั้งและตรวจ boot confirmation แล้วคือ **2.2.3** มีเวลา reset ใต้ usage แต่ละช่วงของ Claude และ Codex แยกตามข้อมูลจริง โหมด Codex Weekly ใช้เวลา reset ของช่วง Weekly ที่เลือก

Build, native UI tests และ app-only OTA ผ่านในเครื่องพัฒนา การตรวจเหล่านี้ไม่ยืนยัน live data ของทุก provider หรือหน้าตา LCD บนบอร์ดทุกตัว

## งานที่รับไว้และยังไม่เสร็จ

- จัดกลุ่ม usage ของ Claude / Codex ให้อยู่กึ่งกลางแนวตั้ง โดยเฉพาะ Codex โหมด Weekly ที่มีหนึ่งช่วง
- เปลี่ยนหัวข้อหน้าอากาศจาก Weather เป็นชื่อสถานที่จาก config เช่น Bangkok และอัปเดตเมื่อเปลี่ยนสถานที่

สองรายการนี้ยังไม่รวมใน firmware 2.2.3 หยุดไว้ระหว่างเตรียม repository สำหรับ GitHub

## GitHub

Repository: [peerapach/desk-duo-monitor](https://github.com/peerapach/desk-duo-monitor)

เก็บ source snapshot ของ firmware 2.2.3 พร้อมคู่มือและ tests ไว้ที่ branch `main` ส่วนงาน UI ด้านบนยังอยู่ในคิว
