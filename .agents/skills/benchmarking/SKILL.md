---
name: benchmarking
description: ใช้เตรียมและรายงาน benchmark ที่ทำซ้ำได้โดยไม่กล่าวอ้างผลที่ยังไม่ได้วัด
---

# Benchmarking

## ขั้นตอน
1. ระบุคำถามที่ benchmark จะตอบและ baseline ที่เปรียบเทียบ
2. บันทึก compiler/version, flags, OS, hardware, dataset, seed และ input size
3. แยก warm-up, repetitions และวิธีสรุปผล; ตรวจ overhead ของ harness
4. สร้าง correctness check ก่อนจับเวลา
5. รันจริงหลายครั้งและบันทึกข้อมูลดิบ/คำสั่ง
6. รายงาน variability, limitations และห้าม extrapolate เกินข้อมูล

## ตรวจคุณภาพ
- benchmark วัดงานที่ระบุและผลลัพธ์ถูกต้อง
- เงื่อนไขทดลองทำซ้ำได้
- ไม่มีตัวเลขที่แต่งขึ้นหรือเลือกเฉพาะ run ที่ดูดี
- แยก measurement จาก interpretation

## พลาดบ่อย
จับเวลา build แทน runtime; ใช้ input เล็กเกิน; ลืม optimization flags; รายงานค่าเดียวโดยไม่บอก variance

## เกณฑ์รับ
รายงานครบตาม template พร้อม command, environment และผลดิบที่ตรวจสอบได้
