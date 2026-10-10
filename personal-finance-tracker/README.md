# Personal Finance Tracker — ML data preparation

## ภาษาไทย

แอปมีหน้า **เตรียมข้อมูล ML** สำหรับเก็บข้อมูลก่อนพัฒนาโมเดลจริง โดยตอนนี้ยังไม่ฝึกหรือแสดงผลทำนาย

- **พยากรณ์รายจ่าย:** เตรียมยอดรายจ่ายรายเดือนแยกตามหมวดสำหรับ Linear Regression ต้องมีข้อมูลอย่างน้อย 12 เดือนที่มีรายจ่าย
- **จำแนกธุรกรรม:** เลือกรายจ่ายแล้วกด `Mark normal` หรือ `Mark suspicious` เพื่อบันทึกผลตรวจโดยมนุษย์ ต้องมีอย่างน้อย 50 รายการต่อกลุ่มก่อนเริ่มพัฒนา Logistic Regression
- แก้ไขข้อมูลธุรกรรมแล้ว label เดิมจะถูกล้าง เพื่อไม่ให้ใช้ label ที่อาจไม่ตรงกับข้อมูลที่แก้แล้ว

เกณฑ์เหล่านี้เป็นจุดเริ่มต้นสำหรับตรวจความพร้อม ไม่ได้รับประกันคุณภาพโมเดล หลังข้อมูลถึงเกณฑ์ ยังต้องฝึกโมเดล แบ่งข้อมูลประเมินผล และเปรียบเทียบค่าคาดการณ์กับยอดจริงก่อนเปิดใช้

ฐานข้อมูลจริงอยู่ใน `%LOCALAPPDATA%\PersonalFinanceTrackerPython\finance.db` และไม่ควรนำเข้า Git กฎ ignore ยกเว้นไฟล์ฐานข้อมูล SQLite และไฟล์ชั่วคราวไว้แล้ว

เริ่มแอปด้วย `python main.py` และรัน tests ด้วย `python -m unittest discover -s tests -v` จากโฟลเดอร์นี้

## English

The **ML data preparation** page collects the data needed before building real models. It does not train models or show predictions yet.

- **Expense forecasting:** Monthly expenses by category are prepared for Linear Regression. The initial readiness gate is 12 distinct months containing expenses.
- **Transaction classification:** Select an expense and use `Mark normal` or `Mark suspicious` to record a human review. At least 50 reviewed transactions per class are required before developing Logistic Regression.
- Editing a transaction clears its previous label so an outdated review is not reused.

These are initial readiness checks, not a guarantee of model quality. After the data threshold is met, the model still needs training, holdout evaluation, and comparison of forecasts with actual spending before it is enabled.

The live database is stored at `%LOCALAPPDATA%\PersonalFinanceTrackerPython\finance.db` and must stay out of Git. The ignore rules exclude SQLite database and temporary files.

Run the app with `python main.py` and run tests with `python -m unittest discover -s tests -v` from this directory.
