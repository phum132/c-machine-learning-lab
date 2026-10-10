---
name: release-checklist
description: ใช้ตรวจความพร้อม release หลัง implementation, integration, tests, security review และ documentation เสร็จ
---

# Release checklist

## ขั้นตอน
1. ตรวจ scope/version และสถานะ repository โดยไม่ทิ้ง user changes
2. รัน build, test, static checks และ sanitizer ที่รองรับ
3. ทบทวน security findings และ known defects
4. ตรวจ README, examples, formats, limitations และ release notes
5. ตรวจว่าไม่มี secrets, binaries หรือ generated data ที่ไม่ควรเผยแพร่
6. บันทึกผลจริงและรายการที่ยังไม่ผ่าน; หยุด release หาก acceptance criteria ไม่ครบ

## ตรวจคุณภาพ
- ทุกคำสั่งและผลตรวจมีหลักฐาน
- ไม่มี critical unresolved finding โดยไม่เปิดเผย
- dependencies และ platform support ระบุชัด
- benchmark อ้างเฉพาะข้อมูลที่วัดจริง

## พลาดบ่อย
ถือว่า CI config เท่ากับ CI ผ่าน; ละเว้น sanitizer ที่ล้มเหลว; ปล่อย known issue โดยไม่ระบุ; สร้าง tag/commit โดยไม่ได้รับอนุญาต

## เกณฑ์รับ
checklist ครบหรือมี blocker ระบุเจ้าของและหลักฐาน พร้อมการอนุมัติ release ตาม workflow
