---
name: csv-parsing
description: ใช้พัฒนา CSV reader, numeric dataset loader และ validation สำหรับ input ภายนอก
---

# อ่าน CSV

## ขั้นตอน
1. ตกลง syntax ที่รองรับ เช่น delimiter, header, quote, escape, newline และ encoding
2. กำหนด limits ของ file, row, column และ field ก่อนจัดสรร
3. ออกแบบผลลัพธ์กรณี empty file, blank row, missing field และ malformed input
4. Parse ตัวเลขโดยตรวจ conversion เต็ม field, range, NaN และ infinity
5. จัดการ I/O error และ cleanup ในทุกเส้นทาง
6. เพิ่ม fixtures ปกติ/ผิดรูปแบบ/ขนาดใหญ่ แล้วบันทึกข้อจำกัด

## ตรวจคุณภาพ
- ไม่มี allocation โดย blind trust ต่อขนาดภายนอก
- row width และ feature/target selection ผ่าน validation
- syntax ที่ไม่รองรับถูกปฏิเสธอย่างชัดเจน
- errors ระบุสาเหตุและตำแหน่งเท่าที่ API รองรับ

## พลาดบ่อย
แยกด้วย comma อย่างเดียวทั้งที่อ้างว่ารองรับ quote; รับ `strtod` ที่แปลงเพียงบางส่วน; กลืน blank/missing values; อ่าน file ทั้งก้อนโดยไม่จำกัดขนาด

## เกณฑ์รับ
grammar และ limits ระบุในเอกสาร, parser ปฏิเสธ invalid inputs อย่างปลอดภัย, tests ครอบคลุม I/O และ parsing errors
