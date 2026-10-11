---
name: git-workflow
description: ใช้ทำงานกับ Git อย่างปลอดภัย ตรวจสถานะและ diff โดยรักษาการเปลี่ยนแปลงที่มีอยู่
---

# Git workflow

## ขั้นตอน
1. ตรวจ root, branch, status และ diff ก่อนเริ่ม
2. แยกไฟล์ที่งานเป็นเจ้าของจาก changes เดิมของผู้ใช้
3. ทำงานเฉพาะ scope และตรวจ status/diff ซ้ำ
4. ตรวจ generated files, secrets, binaries และไฟล์ชั่วคราว
5. สรุปไฟล์ที่เปลี่ยนและสถานะจริง
6. commit/push เฉพาะเมื่อได้รับอนุญาตโดยตรง

## ตรวจคุณภาพ
- ไม่มี unrelated changes ถูกเขียนทับหรือลบ
- ไม่ใช้ reset/clean เพื่อจัดการสถานะ
- ไม่มี secret หรือ artifact ที่ไม่ตั้งใจ
- รายงานสถานะตรงกับผลคำสั่ง

## พลาดบ่อย
สมมติว่า cwd เป็น repository; ใช้ `git clean`; stage ทุกไฟล์โดยไม่ตรวจ; สรุปว่า working tree สะอาดทั้งที่ไม่มี Git metadata

## เกณฑ์รับ
สถานะก่อน/หลังและ diff ตรวจแล้ว; ไม่มี commit/push โดยไม่ได้รับคำสั่ง
