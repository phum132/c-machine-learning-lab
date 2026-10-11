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

`--target` ใช้ column index เริ่มจากศูนย์; prediction CSV ต้องไม่มี header และมีเฉพาะ feature columns. Linear model พิมพ์หนึ่งค่าต่อแถว. Logistic model พิมพ์ `probability,class` และรับ `--threshold` (ค่าเริ่มต้น `0.5`). ดู `.\build\cml-lab.exe --help` สำหรับ syntax ทั้งหมด

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
- [Dataset](include/dataset.h): strict numeric CSV loader พร้อมเลือก target column
- [Serialization](include/model_serialization.h): versioned ASCII model save/load
- [CLI](include/app_cli.h): train, save, load และ batch prediction จาก CSV

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

- Training CLI ใช้ทุกแถวที่ให้มา; ไม่มี CLI option สำหรับ normalization หรือ validation split
- Logistic CLI ยังไม่เปิด metrics/batch evaluation
- Model writer ยังไม่เขียนไฟล์แบบ atomic; serialization ต้องใช้ C numeric locale
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
