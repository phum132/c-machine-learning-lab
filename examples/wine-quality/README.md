# Wine Quality demo / ตัวอย่างพยากรณ์คุณภาพไวน์

## ภาษาไทย

ตัวอย่างนี้ฝึก Linear Regression ด้วย C Machine Learning Lab เพื่อพยากรณ์คะแนนคุณภาพไวน์ (0–10) จากผลตรวจทางเคมี 11 รายการและชนิดไวน์ ใช้ข้อมูลสาธารณะจาก UCI จำนวน 6,497 แถว แบ่ง train/test แบบสุ่มที่ seed คงที่ 80/20 และคำนวณ mean/std จาก train เท่านั้นก่อนปรับสเกลข้อมูล

ต้องมี Python 3 และ build CLI ก่อน โดยไม่ต้องติดตั้งแพ็กเกจ Python เพิ่ม:

```powershell
python examples/wine-quality/run_demo.py
```

สคริปต์จะดาวน์โหลดข้อมูลจาก UCI หากไฟล์ต้นทางยังไม่มี แล้วเรียก CML train/predict ให้จริง สร้างกราฟ `examples/wine-quality/prediction.svg` และรายงาน MAE, RMSE, R² ใน terminal กับ `build/wine-quality/metrics.txt` เปิดไฟล์ SVG ใน browser เพื่อดูกราฟจริง

กราฟเปรียบเทียบคะแนนจริงกับคะแนนที่โมเดลทำนายจาก test set ที่กันไว้ ไม่ได้ใช้ train set มาวัดผล แต่ผลนี้เป็น baseline เชิงสาธิต ไม่ใช่คำแนะนำสำหรับการซื้อขายไวน์หรือการตัดสินใจจริง

## English

This example trains the C Machine Learning Lab's Linear Regression model to predict wine quality scores (0–10) from 11 physicochemical measurements plus a wine-type feature. It uses 6,497 public UCI records, a deterministic 80/20 train/test split, and scaling statistics computed from training data only.

With Python 3 and the CLI built, run this without installing Python packages:

```powershell
python examples/wine-quality/run_demo.py
```

The script downloads the source files from UCI if needed, invokes the CML CLI to train and predict, then writes `examples/wine-quality/prediction.svg` and reports MAE, RMSE, and R² in the terminal and `build/wine-quality/metrics.txt`. Open the SVG in a browser to view the chart.

The chart compares predictions against held-out test records. This is an educational baseline, not a tool for real-world wine purchasing or business decisions.

## Data source and attribution

Paulo Cortez, A. Cerdeira, F. Almeida, T. Matos, and J. Reis, “Modeling wine preferences by data mining from physicochemical properties,” *Decision Support Systems*, 47(4), 547–553, 2009. [UCI Wine Quality dataset](https://doi.org/10.24432/C56S3T), licensed CC BY 4.0. The bundled raw CSV files are unchanged UCI source data.
