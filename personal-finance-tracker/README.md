# Personal Finance Tracker — ML และพยากรณ์รายจ่าย

## ภาษาไทย

แอปมีหน้า **ML และพยากรณ์** สำหรับพยากรณ์รายจ่ายและเก็บข้อมูลสำหรับโมเดลจำแนกธุรกรรม

- **พยากรณ์รายจ่าย:** กด `ฝึกโมเดลและพยากรณ์` แอปจะฝึก Linear Regression ด้วย `cml-lab.exe` จากยอดรายจ่ายรายเดือนแยกหมวด วัดผลกับ 3 เดือนล่าสุดเทียบกับการเดาแบบง่าย แล้วใช้วิธีที่แม่นที่สุด ต้องมีประวัติอย่างน้อย 8 เดือน (แนะนำ 12 เดือนขึ้นไป) วิธีใช้อยู่ใน [คู่มือพยากรณ์รายจ่าย](docs/ml-forecast-guide.md)
- **จำแนกธุรกรรม:** เลือกรายจ่ายแล้วกด `Mark normal` หรือ `Mark suspicious` เพื่อบันทึกผลตรวจโดยมนุษย์ ต้องมีอย่างน้อย 50 รายการต่อกลุ่มก่อนเริ่มพัฒนา Logistic Regression (ยังไม่ฝึกโมเดลนี้)
- แก้ไขข้อมูลธุรกรรมแล้ว label เดิมจะถูกล้าง เพื่อไม่ให้ใช้ label ที่อาจไม่ตรงกับข้อมูลที่แก้แล้ว

การพยากรณ์ต้อง build `cml-lab.exe` ก่อน (`build.bat` ในโฟลเดอร์หลักของ repo) หรือกำหนด `CML_LAB_EXE` เป็น path ของโปรแกรม ข้อมูลไม่ออกจากเครื่อง

ฐานข้อมูลจริงอยู่ใน `%LOCALAPPDATA%\PersonalFinanceTrackerPython\finance.db` และไม่ควรนำเข้า Git กฎ ignore ยกเว้นไฟล์ฐานข้อมูล SQLite และไฟล์ชั่วคราวไว้แล้ว

เริ่มแอปด้วย `python main.py` และรัน tests ด้วย `python -m unittest discover -s tests -v` จากโฟลเดอร์นี้ (integration test ของการพยากรณ์จะข้ามไปถ้ายังไม่ได้ build CLI)

## English

The **ML และพยากรณ์** (ML and forecast) page forecasts expenses and collects data for a future transaction classifier.

- **Expense forecasting:** `ฝึกโมเดลและพยากรณ์` trains Linear Regression with `cml-lab.exe` on monthly expense totals per category, evaluates it on the latest three months against simple baselines, and uses whichever method has the lowest error. It needs at least 8 months of history (12+ recommended). See the [forecast guide](docs/ml-forecast-guide.md) (Thai).
- **Transaction classification:** Select an expense and use `Mark normal` or `Mark suspicious` to record a human review. At least 50 reviewed transactions per class are required before developing Logistic Regression; that model is not trained yet.
- Editing a transaction clears its previous label so an outdated review is not reused.

Forecasting requires the CLI built with `build.bat` at the repository root, or `CML_LAB_EXE` set to its path. Data stays on the machine.

The live database is stored at `%LOCALAPPDATA%\PersonalFinanceTrackerPython\finance.db` and must stay out of Git. The ignore rules exclude SQLite database and temporary files.

Run the app with `python main.py` and run tests with `python -m unittest discover -s tests -v` from this directory. The forecast integration test is skipped when the CLI has not been built.
