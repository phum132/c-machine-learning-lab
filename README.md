# C Machine Learning Lab

โครงการตัวอย่างสำหรับเรียนรู้การสร้างไลบรารี machine learning ด้วย C11 ครอบคลุม matrix, linear/logistic regression, numeric CSV, model serialization, CLI และการตรวจคุณภาพ โค้ด C ไม่มี third-party library dependency

## สถานะโครงการ

Phase A–I เสร็จแล้ว. Fresh GitHub clone ผ่าน Windows build/test และ GitHub Actions CI ผ่านบน commit ล่าสุด โปรดดู [roadmap](docs/implementation-roadmap.md), [handoff Phase H](docs/phase-h-handoff.md) และ [handoff Phase I](docs/phase-i-handoff.md) สำหรับผลจริงและข้อจำกัด

## Prerequisites

- C11 compiler
- Python 3.11 ขึ้นไป สำหรับ CLI/infrastructure tests และ benchmark
- GNU Make สำหรับ `make` targets บน POSIX
- Windows ใช้ `build.bat` กับ GCC; มีสคริปต์ติดตั้ง WinLibs GCC ที่ `scripts/install_windows_toolchain.cmd`

Compiler flags เริ่มต้นคือ `-std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow`.

## Build และ test

บน Linux/macOS หรือ POSIX shell:

```sh
make
make test
```

บน Windows PowerShell หรือ Command Prompt:

```bat
build.bat
build.bat test
```

`make test` และ `build.bat test` สร้าง CLI/test executable แล้วรัน unit tests, CLI subprocess tests และ infrastructure validation.

## ทดลอง train และ predict

ตัวอย่างใน `examples/` มี feature `x` และ target `y = 2x + 1`:

```bat
.\build\cml-lab.exe train --type linear --data examples\linear-train.csv --target 1 --header --model build\linear.cml --iterations 3000 --learning-rate 0.05
.\build\cml-lab.exe predict --model build\linear.cml --input examples\linear-features.csv
```

`--target` ใช้ column index เริ่มจากศูนย์; prediction CSV ต้องไม่มี header และมีเฉพาะ feature columns. Linear model พิมพ์หนึ่งค่าต่อแถว. Logistic model พิมพ์ `probability,class` และรับ `--threshold` (ค่าเริ่มต้น `0.5`). หลัง train เสร็จ CLI พิมพ์ metrics ของ training set ด้วย. ดู `.\build\cml-lab.exe --help` สำหรับ syntax ทั้งหมด

### Normalization, validation split และ evaluate

```bat
.\build\cml-lab.exe train --type linear --data examples\linear-train.csv --target 1 --header --model build\linear.cml --iterations 3000 --learning-rate 0.1 --normalize --validation-split 0.2 --seed 7
.\build\cml-lab.exe evaluate --model build\linear.cml --data examples\linear-train.csv --target 1 --header
```

- `--normalize` คำนวณ mean และ population std ของแต่ละ feature จากแถว train เท่านั้น แล้วเก็บไว้ในไฟล์โมเดล (format version 2). `predict` และ `evaluate` ปรับสเกล input ให้อัตโนมัติ จึงส่ง feature ดิบได้เลย. Feature ที่ค่าคงที่ใช้ scale `1`
- `--validation-split F` (`0 < F < 1`) สุ่มแยกแถวไว้เป็น validation ด้วย `--seed` (ค่าเริ่มต้น `42`) แล้วพิมพ์ metrics ทั้ง `Training:` และ `Validation:`. `--seed` ใช้ได้เฉพาะกับ `--validation-split`
- `evaluate` อ่าน CSV ที่มี target แล้วพิมพ์ MAE/RMSE/R² สำหรับ linear (R² เป็น `n/a` เมื่อ target ค่าคงที่) หรือ accuracy/precision/recall/F1 และ confusion counts สำหรับ logistic (รับ `--threshold`)

### L2 regularization และ k-fold cross-validation

```bat
.\build\cml-lab.exe cross-validate --type linear --data examples\linear-train.csv --target 1 --header --folds 5 --normalize --l2 0.01 --iterations 3000 --learning-rate 0.1
```

- `--l2 X` (ใช้ได้ทั้ง `train` และ `cross-validate`) ลงโทษน้ำหนักที่ใหญ่เกินไป เพื่อกันโมเดลจำข้อมูลฝึกแทนที่จะเรียนรูปแบบ (overfitting) loss ที่ฝึกคือ MSE หรือ cross-entropy บวก `X × ผลรวมน้ำหนักยกกำลังสอง` โดยไม่ลงโทษ bias ค่าเริ่มต้น 0 = ไม่ใช้
- `cross-validate` แบ่งข้อมูลเป็น K ส่วน (`--folds`, ค่าเริ่มต้น 5, สูงสุด 20) ฝึกด้วย K−1 ส่วนแล้วทดสอบส่วนที่เหลือ วนจนครบทุกส่วน พิมพ์ผลแต่ละ fold และค่าเฉลี่ย ± ส่วนเบี่ยงเบนมาตรฐาน `--normalize` ปรับสเกลจากแถวฝึกของแต่ละ fold เท่านั้น `--seed` เปลี่ยนการสุ่มแบ่ง (ค่าเริ่มต้น 42) คำสั่งนี้ไม่บันทึกโมเดล ใช้เลือกค่า เช่น `--l2` หรือ `--learning-rate` ก่อนรัน `train` จริง

Exit code: `0` สำเร็จ, `1` runtime/data/model error, `2` arguments ไม่ถูกต้อง. CSV parser ใช้ grammar แบบ strict และจำกัดขนาด input; รายละเอียดอยู่ใน [ADR-001](docs/adr-001-dataset-csv.md). Model format และ limits อยู่ใน [ADR-003](docs/adr-003-model-serialization.md).

## เดโมข้อมูลจริง

สคริปต์ Python (standard library เท่านั้น) แบ่ง train/test แบบ seed คงที่, ปรับสเกลจาก train เท่านั้น แล้วเรียก CLI จริงเพื่อ train/predict และรายงาน MAE, RMSE, R²:

- [Thai O-NET](examples/thai-onet/README.md) — ทำนายคะแนนเฉลี่ยวิทยาศาสตร์ ม.3 ของโรงเรียนใน กทม. (101 โรงเรียน) — `python examples/thai-onet/run_demo.py`
- [Wine Quality](examples/wine-quality/README.md) — ทำนายคุณภาพไวน์จากข้อมูล UCI 6,497 แถว — `python examples/wine-quality/run_demo.py`

ใช้ CSV ของตัวเองผ่าน `scripts/run_linear_csv.py`; ขั้นตอนอยู่ใน [คู่มือใช้ CSV ของคุณ](docs/using-your-csv.md). ผลเดโมเป็น baseline เชิงสาธิต ไม่ใช่ข้อสรุปเชิงสถิติ

## Personal Finance Tracker

[`personal-finance-tracker/`](personal-finance-tracker/README.md) เป็นแอป Python (tkinter + SQLite) แยกจากไลบรารี C ใช้เก็บและเตรียมข้อมูลรายจ่ายสำหรับโมเดลในอนาคต ยังไม่เรียก CML CLI หรือแสดงผลทำนาย; รายละเอียดอยู่ใน [ML readiness](personal-finance-tracker/docs/ml-readiness.md).

## Features และ API

- [Matrix](include/matrix.h): contiguous row-major storage, add/subtract/multiply/scale/transpose/dot product; operations คืน allocation ใหม่
- [Linear regression](include/linear_regression.h): prediction, MSE, full-batch gradient descent และ seeded train/validation split
- [Logistic regression](include/logistic_regression.h): binary labels, stable sigmoid/cross-entropy และ probability/class prediction
- [Classification metrics](include/classification_metrics.h): accuracy, precision, recall, F1 และ confusion counts
- [Regression metrics](include/regression_metrics.h): MAE, RMSE และ R²
- [Cross-validation](include/cross_validation.h): แบ่งแถวเป็น K fold แบบสุ่มที่ทำซ้ำได้ด้วย seed
- [Feature scaling](include/feature_scaling.h): fit/apply standardization แบบ mean/std
- [Dataset](include/dataset.h): strict numeric CSV loader พร้อมเลือก target column
- [Serialization](include/model_serialization.h): versioned ASCII model save/load (v1 parameters, v2 รวม scaling) เขียนผ่านไฟล์ชั่วคราวแล้วค่อยแทนที่
- [CLI](include/app_cli.h): train (normalize/validation split), evaluate และ batch prediction จาก CSV

API ใช้ `MLStatus` ส่งผลลัพธ์; output object ที่สร้างสำเร็จเป็นของ caller และต้องทำลายด้วยฟังก์ชันคู่กัน ส่วน getter ที่ระบุ borrowed view ยังคงมีอายุเท่ากับ owner ดูรายละเอียด ownership และ module flow ใน [architecture](docs/architecture.md).

## Quality checks และ benchmark

บน POSIX มี `make test-analyzer`, `make test-sanitize` (GCC `-fanalyzer`, ASan/UBSan) และ `make benchmark`. บน Windows ใช้ Clang จาก MSYS2 กับ sanitizer ได้:

```powershell
$env:CC_OVERRIDE = 'C:\msys64\clang64\bin\clang.exe'
$env:CFLAGS_OVERRIDE = '-std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -fsanitize=address,undefined -fno-omit-frame-pointer'
$env:LDFLAGS_OVERRIDE = '-fsanitize=address,undefined'
.\build.bat test
Remove-Item Env:CC_OVERRIDE, Env:CFLAGS_OVERRIDE, Env:LDFLAGS_OVERRIDE
```

Phase H handoff บันทึกผลที่รันจริง. Benchmark เป็นผลจากเครื่องเดียวและข้อมูลสังเคราะห์; ดู [วิธีวัดและผลดิบ](docs/benchmark-phase-h.md).

## Known limitations

- การฝึกเป็น full-batch gradient descent (อ่านข้อมูลทั้งชุดรอบละครั้ง); เฟส K ทำให้เร็วขึ้น 2–3 เท่าโดยผลเหมือนเดิมทุกไบต์ แต่ยังไม่มี mini-batch
- Validation split เป็นการสุ่มแบบครั้งเดียว ไม่ stratify label และยังไม่มี k-fold
- Model writer แทนที่ไฟล์ด้วย rename/MoveFileEx แต่ไม่ fsync จึงไม่รับประกันข้อมูลเมื่อไฟดับ; ถ้าเซฟไฟล์เดียวกันพร้อมกัน ไฟล์ที่เขียนเสร็จทีหลังจะเป็นตัวที่เหลืออยู่; serialization ต้องใช้ C numeric locale
- เดโมใน `examples/` และ `scripts/run_linear_csv.py` ยังปรับสเกลใน Python เอง ไม่ได้ใช้ `--normalize`
- GCC MSYS2 ที่ใช้ในเครื่อง Windows นี้ไม่มี sanitizer runtime; ASan/UBSan ผ่านด้วย MSYS2 Clang แทน
- CI run ที่ยืนยันล่าสุดเป็นผลของ commit `616b959`; รันใหม่เมื่อมี commit ถัดไป

## Project documents

- [Architecture](docs/architecture.md)
- [Implementation roadmap](docs/implementation-roadmap.md)
- [Coding conventions](docs/coding-conventions.md)
- [Agent workflow](docs/agent-workflow.md)
- [Phase H handoff](docs/phase-h-handoff.md)
- [Phase I handoff](docs/phase-i-handoff.md)
- [ใช้ไฟล์ CSV ของคุณ](docs/using-your-csv.md)
