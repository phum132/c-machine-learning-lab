---
name: c-development
description: ใช้เมื่อเพิ่มหรือปรับ C11 API, source, header หรือโครงสร้างพื้นฐานใน repository นี้
---

# พัฒนา C11

## ขั้นตอน
1. อ่าน `AGENTS.md`, task contract, owner files และ build instructions ที่มีจริง
2. กำหนด API, constness, ownership, lifetime และ error behavior ก่อน implementation
3. แยก declaration/implementation ใช้ include guard และจำกัด dependency
4. ตรวจ pointer, dimensions, arithmetic และ cleanup ทุก error path
5. เพิ่ม tests ตาม contract แล้ว build/test ด้วยคำสั่งที่มีอยู่จริง
6. ตรวจ diff และส่ง handoff พร้อมผลตรวจจริง

## ตรวจคุณภาพ
- C11 portable; warning ที่เปิดใช้ไม่มีเหตุให้ละเลย
- ไม่มี overflow, uninitialized read, leak, double-free หรือ ownership กำกวม
- มี tests สำหรับ success, boundary, invalid input และ regression ตามความเหมาะสม
- ไม่มีไฟล์นอก scope หรือ claim ที่ทดสอบไม่ได้

## พลาดบ่อย
คืน pointer โดยไม่บอกว่าใคร free; คูณขนาดก่อนตรวจ overflow; กลืน error; แก้ public header โดยไม่ประสาน consumer

## เกณฑ์รับ
API และ ownership ชัด, implementation ผ่าน contract, tests/build ได้รับการรันหรือระบุข้อจำกัดอย่างตรงไปตรงมา, handoff ครบ
