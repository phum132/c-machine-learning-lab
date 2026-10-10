---
name: ml-from-scratch
description: ใช้ implement หรืออธิบาย Linear/Logistic Regression, gradient descent, prediction และ metrics จากพื้นฐาน
---

# Machine Learning จากพื้นฐาน

## ขั้นตอน
1. ยืนยัน Dataset/Matrix API, feature/target shape, label encoding และ ownership
2. ระบุ objective, gradient, initialization, learning rate, stopping rule และ error behavior
3. ป้องกันข้อมูลไม่ finite และ dimension mismatch
4. ใช้สูตรที่เสถียรและแยก prediction ออกจาก training state
5. ตรวจผลบน deterministic toy data ที่คำนวณคาดหมายได้
6. บันทึกข้อจำกัดและรัน tests จริง

## ตรวจคุณภาพ
- สูตรและ convention อธิบายได้
- ไม่เกิด NaN/Infinity โดยไม่รายงาน
- training behavior และ convergence criteria ชัด
- metrics จัดการ empty input และหารด้วยศูนย์ตาม contract

## พลาดบ่อย
ใช้ sigmoid ที่ overflow; สลับ label/feature; รายงาน training loss เป็นผล validation; อ้าง convergence โดยไม่มีเกณฑ์

## เกณฑ์รับ
สมการตรง implementation, tests deterministic และ edge cases สอดคล้องกับ API ที่ตกลง
