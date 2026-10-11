# การเตรียมข้อมูล ML / ML data preparation

## ภาษาไทย

Tracker ใช้ข้อมูลกับโมเดล 2 แบบ:

- พยากรณ์รายจ่ายใช้ยอดรายจ่ายรายเดือนกับ Linear Regression **ใช้งานได้แล้ว** ดู [คู่มือพยากรณ์รายจ่าย](ml-forecast-guide.md) เกณฑ์ความพร้อมแนะนำ 12 เดือน ส่วนขั้นต่ำที่ระบบยอมฝึกคือ 8 เดือน
- จำแนกธุรกรรมน่าสงสัยจะใช้ Logistic Regression ต้องมีรายจ่ายที่มนุษย์ตรวจแล้วอย่างน้อย 50 รายการต่อกลุ่ม (`ปกติ` และ `น่าสงสัย`)

เกณฑ์นี้เป็นเพียงจุดตรวจความพร้อม ไม่รับประกันว่าข้อมูลเป็นตัวแทนที่ดี เมื่อถึงเกณฑ์แล้วยังต้องฝึกโมเดล ประเมินกับข้อมูลกันไว้ และเทียบค่าคาดการณ์กับยอดจริงก่อนเปิดใช้

Label เป็นค่าว่างได้และใช้กับรายจ่ายเท่านั้น: `0` คือปกติ, `1` คือน่าสงสัย และ `NULL` คือยังไม่ตรวจ แอปไม่สร้าง label หรือคำตัดสินความเสี่ยงให้อัตโนมัติ

## English

The tracker uses its data for two models:

- Expense forecasting uses monthly expense totals with Linear Regression and is implemented; see the [forecast guide](ml-forecast-guide.md). The readiness gate recommends 12 expense months; training requires at least 8.
- Suspicious-transaction classification will use Logistic Regression. It needs at least 50 manually reviewed expense transactions in each class (`normal` and `suspicious`).

These thresholds are conservative implementation gates, not a guarantee that the data is statistically representative. When a threshold is met, model training, holdout evaluation, and comparison against actual spending still need to be implemented and reviewed before displaying predictions or risk scores.

Labels are nullable and only apply to expense transactions. `0` means normal, `1` means suspicious, and `NULL` means not reviewed. The database initializer adds this column to an existing tracker database while preserving its transactions. The application does not infer labels automatically.

The expense-history service returns monthly totals by category and fills missing category-month pairs with zero. The forecast starts its history window at the first recorded expense, so months before tracking began are not treated as zero-spend months.
