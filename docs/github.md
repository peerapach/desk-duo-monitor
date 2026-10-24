# Publish to GitHub

Repository: [peerapach/desk-duo-monitor](https://github.com/peerapach/desk-duo-monitor)

Product name: **Desk Duo**

Description:

> A portrait ESP32 desk companion for Claude & Codex usage, weather, and Thai/US stocks — with bilingual web admin and Wi-Fi OTA.

ใช้ Git repository นี้เก็บ source, generated assets และ tests ส่วน firmware binary ให้แจกผ่าน Releases ภายหลัง ไฟล์ config ส่วนตัวและผล build ถูก ignore ไว้แล้ว

โปรเจกต์ยังไม่ได้กำหนด project license แยกจากใบอนุญาต third-party ที่เก็บไว้แล้ว

## หากจะ push เอง

หลังตรวจและ commit การเปลี่ยนแปลงที่ต้องการแล้ว:

```sh
git remote add origin https://github.com/peerapach/desk-duo-monitor.git
git push -u origin HEAD:main
```

ใช้ `remote add` เฉพาะเมื่อยังไม่มี origin คำสั่ง push ส่ง branch ปัจจุบันไปที่ `main` โดยไม่เปลี่ยนชื่อ branch ในเครื่องและไม่ใช้ force push

ก่อน commit ตรวจ `git status --short` และ `git diff --cached` ให้ภาพ/ฟอนต์ต้นฉบับ, generated sources, modules และ tests ที่เพิ่มใหม่ไปครบ อย่าใช้ `git add -f` กับไฟล์ที่ถูก ignore
