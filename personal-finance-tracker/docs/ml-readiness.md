# การเตรียมข้อมูล ML / ML data preparation

## ภาษาไทย

Tracker เตรียมข้อมูลสำหรับโมเดลในอนาคต 2 แบบ โดยยังไม่ฝึกหรือแสดงผลทำนาย:

- พยากรณ์รายจ่ายจะใช้ยอดรายจ่ายรายเดือนกับ Linear Regression เกณฑ์เริ่มต้นคือมีอย่างน้อย 12 เดือนที่มีรายจ่าย
- จำแนกธุรกรรมน่าสงสัยจะใช้ Logistic Regression ต้องมีรายจ่ายที่มนุษย์ตรวจแล้วอย่างน้อย 50 รายการต่อกลุ่ม (`ปกติ` และ `น่าสงสัย`)

เกณฑ์นี้เป็นเพียงจุดตรวจความพร้อม ไม่รับประกันว่าข้อมูลเป็นตัวแทนที่ดี เมื่อถึงเกณฑ์แล้วยังต้องฝึกโมเดล ประเมินกับข้อมูลกันไว้ และเทียบค่าคาดการณ์กับยอดจริงก่อนเปิดใช้

Label เป็นค่าว่างได้และใช้กับรายจ่ายเท่านั้น: `0` คือปกติ, `1` คือน่าสงสัย และ `NULL` คือยังไม่ตรวจ แอปไม่สร้าง label หรือคำตัดสินความเสี่ยงให้อัตโนมัติ

## English

The tracker now prepares data for two future models without making predictions:

- Expense forecasting will use monthly expense totals with Linear Regression. The initial readiness gate is 12 distinct expense months up to the selected month.
- Suspicious-transaction classification will use Logistic Regression. It needs at least 50 manually reviewed expense transactions in each class (`normal` and `suspicious`).

These thresholds are conservative implementation gates, not a guarantee that the data is statistically representative. When a threshold is met, model training, holdout evaluation, and comparison against actual spending still need to be implemented and reviewed before displaying predictions or risk scores.

Labels are nullable and only apply to expense transactions. `0` means normal, `1` means suspicious, and `NULL` means not reviewed. The database initializer adds this column to an existing tracker database while preserving its transactions. The application does not infer labels automatically.

The expense-history service returns monthly totals by category and fills missing category-month pairs with zero. A future training flow must check that the user's transaction records cover the full history window before treating a month with no expenses as a true zero-spend month.
