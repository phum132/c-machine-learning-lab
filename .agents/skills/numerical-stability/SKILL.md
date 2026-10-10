---
name: numerical-stability
description: ใช้ตรวจและออกแบบสูตร floating-point, loss, sigmoid, reductions และ convergence ในโมเดล ML
---

# ความเสถียรเชิงตัวเลข

## ขั้นตอน
1. ระบุช่วง input/output และนโยบาย NaN/Infinity ของ API
2. วิเคราะห์ overflow, underflow, cancellation และการสะสม error
3. เลือกสูตรที่เสถียร เช่น sigmoid แบบแยกเครื่องหมายและ stable log-sum-exp เมื่อเหมาะสม
4. กำหนด tolerance โดยสัมพันธ์กับ scale และชนิดข้อมูล
5. ทดสอบค่าขนาดใหญ่/เล็ก ขอบเขต และ non-finite values
6. อธิบาย trade-offs และหลักฐานผลทดสอบ

## ตรวจคุณภาพ
- intermediate values ไม่ล้นโดยไม่ตรวจ
- loss/gradient มี finite behavior ที่กำหนด
- tolerance ไม่หลวมจนซ่อน defect
- convergence claims ผูกกับ stopping criteria

## พลาดบ่อย
คำนวณ `log(sigmoid(x))` ตรง ๆ; เปรียบเทียบ float ด้วย equality โดยไม่มีเหตุผล; clamp แล้วไม่บอก; ถือว่า double ป้องกัน numerical issue ทุกกรณี

## เกณฑ์รับ
สูตรและ edge behavior อธิบายได้ และมี test ที่ครอบคลุมช่วงค่าที่เสี่ยง
