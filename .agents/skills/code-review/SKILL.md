---
name: code-review
description: ใช้ทบทวน diff ที่รวมแล้วเพื่อค้น defect และความเสี่ยงด้าน correctness, maintainability, portability หรือ performance
---

# Code review

## ขั้นตอน
1. อ่าน task, acceptance criteria, public contract และ diff จริง
2. ไล่ data/control flow ในบริบท ไม่สรุปจาก patch snippet อย่างเดียว
3. ตรวจ correctness, ownership, error handling, portability, numerical behavior และ tests
4. ยืนยันแต่ละ finding พร้อมตำแหน่งและผลกระทบ
5. แยก confirmed defect จาก risk/ข้อเสนอแนะ
6. ส่งรายงานตาม template โดยไม่แก้ไฟล์ เว้นแต่ได้รับมอบหมาย

## ตรวจคุณภาพ
- finding มี severity, file/line, evidence และผลกระทบ
- ไม่มีข้อกล่าวหาที่ไม่มีหลักฐาน
- ระบุส่วนที่ตรวจและข้อจำกัดของการ review

## พลาดบ่อย
รายงาน style preference เป็น defect; review เฉพาะ summary; สันนิษฐานว่า tests ผ่าน; แนะนำการแก้ที่อยู่นอก scope

## เกณฑ์รับ
รายงานตรวจสอบซ้ำได้จาก integrated implementation และจัดลำดับ finding ตามความรุนแรง
