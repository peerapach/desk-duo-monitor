# Stocks endpoint สำหรับ Google Sheets

สคริปต์ `Code.gs` อ่านตารางหุ้นจาก Google Sheet แล้วส่ง JSON สำหรับ ESP32 โดยไม่ต้องให้บอร์ดอ่าน Sheet โดยตรง ข้อมูลราคาใน Sheet ต้องมาจากการกรอกหรือแหล่งข้อมูลอื่นเอง การดึงข้อมูลอัตโนมัติและการยืนยันว่าราคาเป็นราคาตลาดล่าสุดยังไม่อยู่ในขอบเขตนี้

## รูปแบบ Sheet

แถวแรกต้องเป็น header โดยต้องมี `symbol` และ `price` ชื่อ header ไม่แยกตัวพิมพ์เล็ก/ใหญ่และตัดช่องว่างหัวท้ายก่อนจับคู่ คอลัมน์อื่นที่ไม่รู้จักจะถูกมองข้าม

| Header | จำเป็น | ความหมาย |
| --- | --- | --- |
| `symbol` | ใช่ | สัญลักษณ์หุ้น เช่น `PTT` |
| `price` | ใช่ | ราคาเป็นตัวเลข ไม่รับค่าติดลบ |
| `label` | ไม่ | ชื่อที่แสดง; ถ้าว่างจะใช้ `symbol` |
| `changePercent` | ไม่ | เปอร์เซ็นต์เปลี่ยนแปลง เช่น `1.56` หมายถึง `+1.56%` |
| `currency` | ไม่ | สกุลเงิน; ถ้าว่างจะใช้ `THB` |
| `quotedAt` | ไม่ | เวลาที่แหล่งข้อมูลให้ราคานี้ เป็น Date ใน Sheet หรือ ISO datetime ที่มี timezone |

ตัวอย่างตารางต่อไปนี้เป็นข้อมูลจำลอง ไม่ใช่ราคาตลาดปัจจุบัน:

| symbol | price | changePercent | quotedAt | label | currency |
| --- | ---: | ---: | --- | --- | --- |
| PTT | 32.5 | 1.56 | 2026-10-04T02:55:00Z | PTT | THB |
| AOT | 60 |  |  | Airports of Thailand | THB |

สูตรใน Sheet ใช้ได้เมื่อผลลัพธ์ของเซลล์เป็นตัวเลข JavaScript ตามที่ `getValues()` คืนมา ค่าตัวเลขที่เป็นข้อความรับเฉพาะรูปแบบ decimal/exponent ที่ไม่กำกวม เช่น `32.5` หรือ `3.25e1` ค่าว่างจะไม่ถูกเปลี่ยนเป็นศูนย์ และค่า `0` จะคงอยู่ในคำตอบ สคริปต์ตรวจเฉพาะ header และข้อมูล 200 แถวแรก แล้วส่งหุ้นที่มี symbol ไม่ว่างไม่เกิน 5 รายการตามลำดับแถว

header ที่ขาด `symbol`/`price`, header ที่รู้จักซ้ำ หรือค่าหุ้น/ตัวเลข/วันที่ที่ไม่ถูกต้อง จะได้ JSON error code `invalid_sheet` แทนการส่งราคาที่แต่งขึ้น

## ติดตั้ง Apps Script

1. สร้าง Google Sheet และเก็บเป็นส่วนตัว อย่าเปิดสิทธิ์ให้ทุกคนอ่าน Sheet ต้นทาง
2. เปิด **Extensions → Apps Script** จาก Sheet นั้น เปิด `Code.gs` แล้วแทนที่เนื้อหาด้วยไฟล์ `integrations/google-sheets/Code.gs` ในโปรเจกต์นี้ จากนั้นบันทึก
3. เปิด **Project Settings → Script Properties** แล้วเพิ่มค่าต่อไปนี้:

   | Property | ค่า |
   | --- | --- |
   | `SPREADSHEET_ID` | ส่วนของ URL Sheet ระหว่าง `/d/` และ `/edit` |
   | `SHEET_NAME` | ชื่อแท็บตามที่เห็นใน Sheet ทุกตัวอักษร |
   | `ACCESS_TOKEN` | token สุ่มที่สร้างในเครื่องของคุณ |

   สร้าง token ใหม่ใน Terminal บนเครื่องของคุณด้วย `openssl rand -hex 32` แล้วใส่ค่านั้นใน Script Properties โดยตรง อย่าส่ง token จริงในแชตหรือใส่ไว้ในเอกสารที่แชร์
4. เลือก **Deploy → New deployment → Web app** ตั้ง **Execute as** เป็นบัญชีเจ้าของสคริปต์ แล้วเลือก **Who has access** ตามนโยบายของบัญชีหรือ Workspace หากบอร์ดต้องเรียกโดยไม่ลงชื่อเข้า Google ต้องมีการเข้าถึงเว็บแอปที่ไม่บังคับ Google login; ใช้ได้เฉพาะเมื่อ Workspace อนุญาต การตั้งค่านี้ไม่ทำให้ Sheet ต้นทางเป็นสาธารณะ
5. Deploy แล้วจด URL เว็บแอปที่ลงท้ายด้วย `/exec` ไว้สำหรับตั้งค่า ESP32 โดยยังไม่ต้องนำ token ไปไว้ใน log หรือหน้าจอ

## เรียก endpoint

ส่ง GET ไปยัง URL ที่ได้จากขั้นตอน Deploy พร้อม query parameter `accessToken`:

```text
https://script.google.com/macros/s/DEPLOYMENT_ID/exec?accessToken=YOUR_TOKEN
```

ใส่ token จริงในอุปกรณ์หรือเครื่องมือทดสอบของคุณเองเท่านั้น หาก token มีอักขระพิเศษ ให้ URL-encode ก่อนส่ง ตัว endpoint ตรวจ token ก่อนเรียก `SpreadsheetApp` และรับ token ผ่าน query parameter ตาม event ของ Apps Script เท่านั้น

ตัวอย่าง response อยู่ใน [`examples/stocks-response.json`](../../examples/stocks-response.json) ค่า `generatedAt` คือเวลาที่ endpoint สร้างคำตอบ ส่วน `quotedAt` คือเวลาของราคาจากแหล่งข้อมูลและเป็น optional; หากไม่มีเวลาราคา อย่าสร้าง timestamp ขึ้นเอง ฟิลด์ราคาและ `changePercent` ที่ว่างจะถูกละออก

Apps Script `ContentService` ควบคุม HTTP status code ของ response ไม่ได้ ดังนั้น error อาจมาพร้อม HTTP 200 ให้ผู้เรียกตรวจ JSON และ `error.code` ด้วย รหัสที่ส่งออกมีเพียง `auth_required`, `configuration_error`, `invalid_sheet` และ `source_unavailable`; ข้อความ exception, token และ ID ส่วนตัวจะไม่ถูกส่งกลับ แอปบน ESP32 ต้องตาม redirect ของ Google ที่เกิดจาก ContentService ได้

## หมุน token และดูแลแหล่งราคา

เมื่อต้องหมุน token ให้สร้างค่าใหม่ในเครื่อง แก้ `ACCESS_TOKEN` ใน Script Properties และอัปเดตค่าใน ESP32 ให้ตรงกัน จากนั้น token เก่าจะใช้ไม่ได้ อย่าใส่ token ใน serial log เพราะ URL ที่เรียกมี token อยู่ใน query string

ไฟล์นี้ทำหน้าที่อ่าน Sheet และตรวจรูปแบบข้อมูลเท่านั้น ยังไม่มีการดึงหรือเติมราคาหุ้นอัตโนมัติ เช่น `GOOGLEFINANCE` หรือบริการภายนอก ผู้ใช้ต้องเลือกวิธีเติมราคาที่เหมาะกับหุ้นที่ติดตามและตรวจเวลา `quotedAt` จากแหล่งข้อมูลนั้นเอง
