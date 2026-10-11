# แนวทางออกแบบ

## โครงสร้าง

- `main.py` เป็นจุดเริ่มต้นของแอป
- `finance/ui.py` แสดงหน้าจอ Tkinter และส่งคำสั่งจากผู้ใช้ไปยัง service
- `finance/services.py` จัดการ CRUD, สรุปยอด, กราฟ และงบประมาณ
- `finance/database.py` เปิด SQLite และสร้างตาราง/ดัชนี
- `finance/ml.py` สร้างข้อมูลฝึกจากประวัติรายจ่าย เรียก `cml-lab.exe` ผ่าน subprocess และเลือกวิธีพยากรณ์ที่แม่นที่สุด
- `finance/validation.py` ตรวจสอบและจัดรูปแบบจำนวนเงิน วันที่ เดือน และหมวดหมู่
- `tests/` ตรวจสอบกฎจำนวนเงิน ฐานข้อมูล และพฤติกรรม service โดยไม่เปิดหน้าต่าง UI

## ข้อมูล

ตาราง `transactions` เก็บประเภทรายการ หมวดหมู่ รายละเอียด จำนวนเงินหน่วยสตางค์ และวันที่ ISO `YYYY-MM-DD` ตาราง `budgets` เก็บวงเงินหน่วยสตางค์โดยใช้คู่เดือน/หมวดหมู่เป็นคีย์หลัก

ทุกคำสั่งที่รับค่าจากผู้ใช้ใช้ SQLite placeholders แทนการต่อข้อความ SQL โดยตรง การแก้ไขยอดเงินใช้ integer satang เพื่อไม่ให้เกิดความคลาดเคลื่อนจาก floating point

## กราฟ

หน้า **ภาพรวม** แสดงกราฟแท่งรายรับเทียบรายจ่ายย้อนหลัง 6 เดือน และกราฟแท่งแนวนอนรายจ่ายแยกตามหมวดของเดือนที่เลือก วาดด้วย Tkinter Canvas จึงไม่ต้องติดตั้ง plotting package เพิ่ม

## ที่เก็บฐานข้อมูล

Windows: `%LOCALAPPDATA%\PersonalFinanceTrackerPython\finance.db`
ระบบอื่น: `~/.local/PersonalFinanceTrackerPython/finance.db` (หรือโฟลเดอร์ที่กำหนดผ่าน `LOCALAPPDATA`)

## ML และพยากรณ์

หน้า `ML และพยากรณ์` แสดงความพร้อมของข้อมูล และพยากรณ์รายจ่ายเดือนถัดไปด้วย `cml-lab.exe` การฝึกรันใน worker thread แล้วส่งผลกลับผ่าน queue ที่ main thread อ่าน เพราะ SQLite connection และ Tk widgets ใช้ได้เฉพาะ thread ที่สร้าง ข้อมูลจึงอ่านจากฐานข้อมูลก่อนเริ่ม thread ดูวิธีใช้ที่ [ml-forecast-guide.md](ml-forecast-guide.md) และเกณฑ์ที่ [ml-readiness.md](ml-readiness.md)
