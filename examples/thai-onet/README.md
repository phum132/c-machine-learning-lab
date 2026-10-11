# Thai O-NET school score demo

## ไทย

เดโมนี้ใช้ข้อมูลผล O-NET ชั้น ม.3 ปีการศึกษา 2565 ของโรงเรียนในกรุงเทพมหานครจาก Open Data กรุงเทพมหานคร จำนวน 109 แถว โดยข้อมูลเป็นค่าเฉลี่ยระดับโรงเรียน ไม่ใช่คะแนนรายตัวนักเรียน สคริปต์คัดแถวที่มีข้อมูลครบ เหลือ 101 โรงเรียน แล้วใช้ C Machine Learning Lab ฝึก Linear Regression เพื่อทำนายคะแนนเฉลี่ยวิทยาศาสตร์จากจำนวนผู้เข้าสอบและคะแนนเฉลี่ยภาษาไทย ภาษาอังกฤษ และคณิตศาสตร์

รันจากโฟลเดอร์หลักของ repo ด้วย Python 3 และ CLI ที่ build แล้ว:

```powershell
python examples/thai-onet/run_demo.py
```

สคริปต์แบ่ง train/test แบบสุ่ม seed คงที่, ปรับสเกลโดยอิง train เท่านั้น และเรียก CML CLI จริง ผลกราฟ `prediction.svg` กับผลวัด MAE, RMSE และ R² จะถูกสร้างในโฟลเดอร์นี้

ข้อมูลมีเพียง 101 แถว และเป็นค่าเฉลี่ยระดับโรงเรียนในกรุงเทพฯ ปีเดียว ผลจึงเป็นเดโมการใช้ CLI เท่านั้น ไม่ควรใช้สรุปผลหรือเปรียบเทียบคุณภาพนักเรียน/โรงเรียน

แหล่งข้อมูล: [ผลการทดสอบ O-NET ชั้น ม.3 ปีการศึกษา 2565 — Open Data กรุงเทพมหานคร](https://data.bangkok.go.th/dataset/04b8b8de-6c5a-41c1-9ac0-4390be7bbd37/resource/9186821f-348b-45da-8971-312154a695ac)

## English

This demo uses 109 school-level records of Bangkok's Grade 9 O-NET results for academic year 2022 (B.E. 2565). These are school averages, not individual student scores. After dropping incomplete rows, the script trains CML Linear Regression on 101 schools to predict each school's average science score from the number of test takers and the average Thai, English, and mathematics scores.

From the repository root, with Python 3 and the CML CLI built, run:

```powershell
python examples/thai-onet/run_demo.py
```

The script uses a deterministic train/test split, scales features using training rows only, and invokes the CML CLI. It writes `prediction.svg` and reports MAE, RMSE, and R² in this folder.

The dataset contains only 101 complete school records from Bangkok for one year. Treat the results as a CLI demonstration, not as a basis for evaluating students or schools.

Source: [Bangkok O-NET Grade 9 results, academic year 2022 — Bangkok Open Data](https://data.bangkok.go.th/dataset/04b8b8de-6c5a-41c1-9ac0-4390be7bbd37/resource/9186821f-348b-45da-8971-312154a695ac). The resource page lists the license as Open Data Common.
