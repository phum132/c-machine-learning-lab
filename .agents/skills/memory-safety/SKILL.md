---
name: memory-safety
description: ใช้ทบทวน C allocation, pointer lifetime, buffer bounds, cleanup และข้อมูลไฟล์ที่ไม่น่าเชื่อถือ
---

# ความปลอดภัยของหน่วยความจำ

## ขั้นตอน
1. ระบุเจ้าของ resource ทุกตัว จุดสร้าง จุดโอน ownership และจุดทำลาย
2. ไล่ success/error paths เพื่อหา cleanup ที่ขาดหรือซ้ำ
3. ตรวจขนาดและ integer overflow ก่อน allocation หรือ pointer arithmetic
4. ตรวจ index, buffer length, string termination และขอบเขตจาก input
5. ตรวจ file I/O, truncation และขนาดสูงสุดก่อน parse/allocate
6. เพิ่มกรณีทดสอบที่จับ defect; ใช้ sanitizer เมื่อ toolchain รองรับ

## ตรวจคุณภาพ
- ไม่มี out-of-bounds, use-after-free, double-free, uninitialized read หรือ leak ที่พบ
- การคูณ/บวกขนาดไม่ overflow ก่อนจัดสรร
- error path คืนสถานะชัดและ cleanup ถูกต้อง
- findings มี severity, location และหลักฐาน

## พลาดบ่อย
สมมติว่า `malloc(0)` ให้ผลใช้ได้; ใช้ `realloc` โดยทับ pointer เดิมทันที; เชื่อขนาดที่มาจากไฟล์; คิดว่า test ผ่านหมายถึง memory-safe ทั้งหมด

## เกณฑ์รับ
มีการทบทวน ownership, bounds, overflow และ error paths พร้อมผลตรวจจริงและข้อจำกัดที่ระบุไว้
