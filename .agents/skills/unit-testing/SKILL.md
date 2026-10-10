---
name: unit-testing
description: ใช้สร้างหรือปรับปรุง unit tests และ test harness ของโมดูล C ใน repository นี้
---

# Unit testing

## ขั้นตอน
1. ขอ API/behavior และ acceptance criteria ที่ยืนยันแล้ว
2. เลือก deterministic fixtures และคำนวณ expected values แยกจาก implementation
3. เพิ่ม success, boundary, invalid-input และ regression cases ตาม risk
4. ตรวจว่า test fail เมื่อ behavior ผิด โดยไม่พึ่ง timing/randomness
5. รัน test command ที่มีอยู่จริง; เก็บ exit code/output
6. ตรวจ diff ว่า assertion ไม่ถูกลบหรือผ่อนโดยไม่มีเหตุผล

## ตรวจคุณภาพ
- tests แยกเหตุที่ fail ได้และตรวจ behavior ที่ผู้ใช้สังเกตได้
- cleanup fixtures/resources ถูกต้อง
- expected value และ tolerance มีเหตุผล
- รายงาน command/result จริง

## พลาดบ่อย
ทดสอบ helper ภายในแทน behavior; คาดหวังค่าจาก code เดียวกับที่ทดสอบ; ลด assertion เพื่อให้ผ่าน; ลืม error path

## เกณฑ์รับ
tests deterministic, ตรง contract, รันได้ด้วยคำสั่งที่บันทึก และไม่มีการทำให้ assertion อ่อนลง
