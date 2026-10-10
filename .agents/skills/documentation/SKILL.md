---
name: documentation
description: ใช้ปรับ README, architecture docs, usage examples และคำอธิบายการออกแบบให้ตรงกับโค้ดจริง
---

# เอกสารโครงการ

## ขั้นตอน
1. ตรวจ implementation, CLI help, build/test commands และข้อจำกัดจริง
2. แยก implemented, verified, planned และ known limitations
3. เขียนขั้นตอนที่ทำซ้ำได้ พร้อม prerequisites และ expected output ที่ตรวจแล้ว
4. อธิบาย ownership, errors, numerical behavior และ formats ตาม contract
5. ตรวจ links, paths, commands และตัวอย่างกับ repository
6. ตรวจ diff และส่ง handoff พร้อมสิ่งที่ยังไม่ยืนยัน

## ตรวจคุณภาพ
- ไม่มี feature/test/benchmark claims ที่ไม่มีหลักฐาน
- ตัวอย่างตรง API และ syntax ที่รองรับ
- ข้อจำกัดและ prerequisites ชัด
- ภาษาเหมาะกับผู้เริ่มเรียน

## พลาดบ่อย
คัดลอกแผนงานมาเขียนเป็น feature ที่เสร็จแล้ว; ใช้ command ที่ไม่เคยตรวจ; ปล่อยลิงก์เสีย; ซ่อนข้อจำกัด parser

## เกณฑ์รับ
เอกสารสอดคล้องกับ implementation และตรวจซ้ำได้; claims ที่ยังไม่ยืนยันถูกระบุชัด
