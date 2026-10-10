---
name: matrix-library
description: ใช้พัฒนา/ทบทวน matrix API และ operations โดยเน้น dimension checks, allocation safety และผลคำนวณที่ตรวจได้
---

# ไลบรารี Matrix

## ขั้นตอน
1. อ่าน matrix contract และกำหนด representation, ownership และ zero-size behavior
2. ตรวจ dimensions และ overflow ของ `rows * cols * sizeof(element)` ก่อน allocate
3. ระบุ behavior ของ add/subtract/multiply/transpose/dot เมื่อมิติไม่ตรง
4. ออกแบบ cleanup และ output behavior เมื่อ operation ล้มเหลว
5. เขียน known-value tests และกรณี mismatch, boundary, allocation/error
6. ตรวจ numerical input policy และผล build/test จริง

## ตรวจคุณภาพ
- ทุก operation ตรวจ shape ก่อนอ่านข้อมูล
- ไม่มี signed overflow หรือ out-of-bounds indexing
- ownership และ aliasing behavior ชัด
- expected values คำนวณได้ด้วยมือ

## พลาดบ่อย
สลับ row/column; คิดว่า contiguous layout ปลอด overflow; เขียน output บางส่วนก่อนพบ error โดยไม่ระบุ; ไม่ทดสอบ rectangular matrices

## เกณฑ์รับ
API ตรง contract, shape/error behavior ชัด, tests ครอบคลุม matrix ที่ไม่ใช่จัตุรัสและกรณีผิดพลาด
