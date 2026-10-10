# แผนพัฒนา

ทุกระยะต้องผ่านเกณฑ์ของตนก่อนเริ่มระยะที่ dependency ต่อเนื่อง ระยะ Phase 2 เป็นต้นไปเริ่มหลังเจ้าของโครงการอนุมัติ ผล build/test ต้องบันทึกเฉพาะคำสั่งที่รันจริง

## Phase A — Foundation

**งาน:** build system และโครงสร้าง C, shared error handling, CLI ขั้นต้น, test harness, coding conventions

**เกณฑ์รับ:** compiler/toolchain ถูกตรวจพบและบันทึก; มี build/test commands ที่ทำซ้ำได้; warning profile และ error conventions ถูกกำหนด; smoke test ผ่าน; CI compile/test จริงได้; เอกสาร setup ตรงกับเครื่องมือ

**ความคืบหน้า:** เพิ่ม `MLStatus`, CLI `--help`/`--version`, C test harness, subprocess CLI tests, Makefile, Windows `build.bat` และ CI build/test job แล้ว. วันที่ 2026-10-10 compile ด้วย GCC C11 และรัน unit, subprocess CLI และ infrastructure checks ผ่าน; รายละเอียดอยู่ใน [รายงาน handoff Phase B](phase-b-handoff.md).

## Phase B — Matrix Library

**งาน:** create/destroy, add/subtract, multiplication, scalar multiplication, transpose, dot product, dimensions, allocation failures และ unit tests

**สัญญา API ที่ใช้:** `MLMatrix` เก็บ contiguous row-major data; มิติเป็นบวก; create เป็นเจ้าของ allocation และ destroy คืน memory; inputs ของ operations เป็น borrowed immutable data และผลลัพธ์เป็น allocation ใหม่ซึ่ง caller ทำลาย. Output pointer ถูกตั้งเป็น `NULL` ก่อนทำงานและคงเป็น `NULL` เมื่อ error. `matrix_data` เป็น mutable borrowed pointer. Shape mismatch คืน `ML_STATUS_DIMENSION_MISMATCH`; zero dimensions/overflow/invalid pointer คืน `ML_STATUS_INVALID_ARGUMENT`; non-finite input หรือผลคืน `ML_STATUS_NUMERIC_ERROR`. ไม่มี allocation cap เพิ่มเติมนอกเหนือจาก `SIZE_MAX` และความพร้อมของ allocator.

**สถานะ:** เริ่ม Phase B ตามคำสั่งเจ้าของโครงการแล้ว; implementation, tests, build wiring และ docs เพิ่มแล้ว. Local compile/tests ผ่าน; ยังไม่ตรวจ sanitizer, allocation-failure injection หรือ CI hosted runner.

**เกณฑ์รับ:** public API และ ownership ได้รับอนุมัติ; shape mismatch/error behavior ชัด; มี known-value tests ทั้ง rectangular และ edge cases; overflow ป้องกันก่อน allocate; build/test ผ่าน

## Phase C — Linear Regression

**งาน:** model representation, prediction, MSE, gradient descent, training history, train/validation split และ sample data ที่ทำซ้ำได้

**เกณฑ์รับ:** shapes/initialization/stopping criteria ระบุ; toy dataset มีผลคาดหมายตรวจได้; split ทำซ้ำได้ด้วย seed; training/prediction และ errors มี tests; ไม่มีผล numerical ที่ไม่ finite โดยไม่จัดการ

**สถานะ:** implementation และ local acceptance checks เสร็จ. API ครอบคลุม model ที่เริ่ม weights/bias เป็นศูนย์, prediction, training MSE, full-batch gradient descent, history และ seeded train/validation split. Training คืน MSE ตั้งต้นและหลังแต่ละ update; หยุดเมื่อ absolute change ของ training MSE ไม่เกิน tolerance ติดต่อกันครบ patience หรือถึง max iterations. เกณฑ์นี้ระบุ loss คงที่ ไม่ได้รับประกันว่า loss ลดลงหรือถึง optimum. ข้อมูลที่ไม่ finite ถูกปฏิเสธ/คืน numeric error; หากการ train ล้มเหลว parameters ถูก rollback. รายละเอียดผลตรวจและ compiler diagnostics อยู่ใน [handoff Phase C](phase-c-handoff.md).

**การตรวจรับ:** deterministic toy data `y = 2x + 1`, split ที่ใช้ seed เดิม และ invalid/non-finite/rollback cases อยู่ใน `tests/test_linear_regression.c`; ผลตรวจจริงบันทึกใน [handoff Phase C](phase-c-handoff.md).

## Phase D — Dataset และ CSV

**งาน:** numeric dataset, header, feature/target selection, invalid/missing-value handling และข้อจำกัด parser

**เกณฑ์รับ:** grammar และ limits บันทึก; empty/single-row/malformed/non-finite/I/O cases มี tests; allocation จากข้อมูลภายนอกถูกจำกัด; API เชื่อม model ได้ตาม contract

**สัญญาและสถานะ:** Phase D ผ่าน local acceptance checks. ยอมรับ [ADR-001](adr-001-dataset-csv.md): caller ระบุ header mode และ target column index; `MLDataset` เป็นเจ้าของ features, targets และ optional column names; getters เป็น borrowed read-only. Parser รับ strict numeric CSV เท่านั้นและจำกัดไฟล์ 16 MiB, record 64 KiB, field 4096 bytes, 4096 columns, 100,000 rows และ 2,000,000 cells. รายละเอียด handoff และผลตรวจอยู่ใน [Phase D handoff](phase-d-handoff.md).

## Phase E — Logistic Regression

**งาน:** stable sigmoid, binary cross-entropy, gradient descent, probability/class prediction, accuracy, precision, recall, F1 และ zero-division handling

**เกณฑ์รับ:** สูตร stable และ label convention ระบุ; boundary/large-magnitude values มี tests; threshold และ zero-denominator policy ชัด; ผล deterministic ตรง expected values

**สัญญาและสถานะ:** Phase E ผ่าน local acceptance checks. ยอมรับ [ADR-002](adr-002-logistic-regression.md): labels เป็น 0/1 เท่านั้น, threshold เป็น `[0,1]` และเท่ากับ threshold แล้วทำนาย positive, zero denominator ของ precision/recall/F1 คืน 0. Model ใช้ stable sigmoid, binary cross-entropy แบบ log1p และ full-batch gradient descent พร้อม rollback เมื่อ numeric error. รายละเอียดผลตรวจอยู่ใน [Phase E handoff](phase-e-handoff.md).

## Phase F — Model Serialization

**งาน:** versioned format, validation ของ dimensions/parameters, corrupt-file handling, round-trip tests

**เกณฑ์รับ:** format ระบุ endianness/encoding/version/limits; ไม่ serialize raw pointers; ไม่จัดสรรจาก untrusted size โดยไม่มี cap; ตรวจ truncation/non-finite/unsupported version; round-trip และ corrupt tests ผ่าน

**สถานะ:** เสร็จและผ่าน local acceptance checks. เพิ่ม validated parameter setters สำหรับ model opaque ทั้งสองชนิด และ version 1 ASCII format ที่ใช้ hexadecimal floating-point, จำกัดไฟล์ 16 MiB/features 250,000, ตรวจ type/version/count/truncation/trailing token/non-finite และมี round-trip/corrupt-input tests. สัญญาและข้อจำกัดอยู่ใน [ADR-003](adr-003-model-serialization.md); ผลส่งมอบอยู่ใน [Phase F handoff](phase-f-handoff.md). การเขียนทับไฟล์ยังไม่ atomic และ parser/writer กำหนดให้ใช้ C numeric locale.

## Phase G — CLI Inference

**งาน:** train, save/load model, predict records, usage/error reporting และ process exit codes

**สถานะ:** เสร็จและผ่าน local acceptance checks. CLI มี train/save สำหรับ linear/logistic จาก CSV และ load/predict หลายแถวจาก feature-only CSV; logistic คืน probability และ class ตาม threshold. Exit code 0=success, 1=runtime/data/model error, 2=invalid arguments. Subprocess tests ครอบคลุม end-to-end training, serialization/load, prediction, malformed input, dimension mismatch และ argument error; รายละเอียดอยู่ใน [Phase G handoff](phase-g-handoff.md).

## Phase H — Quality

**งาน:** automated tests, compiler warnings, sanitizers เมื่อรองรับ, static analysis และ reproducible benchmarks

**เกณฑ์รับ:** CI build/test ผ่านบน runner ที่กำหนด; sanitizer/static analyzer ระบุ availability และผลจริง; benchmark methodology/input/settings บันทึกและมีผลวัดจริง; ข้อจำกัดเปิดเผย

**สถานะ:** local quality checks และ reproducible benchmark เสร็จ. `build.bat test` และ forced Windows Make test ผ่าน; GCC `-fanalyzer` ผ่านโดยไม่มี `-Wanalyzer-*` findings หลังแก้ test double-close และเพิ่ม overflow guard. Benchmark methodology/ผลวัดจริงอยู่ใน [benchmark-phase-h](benchmark-phase-h.md). MSYS2 GCC ไม่มี sanitizer runtime สำหรับ Windows; หลังติดตั้ง MSYS2 Clang พร้อม compiler-rt แล้ว `build.bat test` ด้วย ASan/UBSan ผ่าน. CI sanitizer job ตั้งไว้สำหรับ Ubuntu 24.04 แต่ยังไม่มี hosted workflow result จาก workspace นี้. รายละเอียดและคำสั่งอยู่ใน [Phase H handoff](phase-h-handoff.md).

## Phase I — Portfolio

**งาน:** README, architecture, usage examples, verified test results, honest benchmarks, limitations/future work และ GitHub Actions CI

**เกณฑ์รับ:** setup ทำซ้ำได้จาก clean checkout; docs/commands/features ตรง implementation; test และ benchmark claims มีหลักฐาน; CI ผ่านจริง; ไม่มี secret หรือ generated artifact ที่ไม่ตั้งใจ

**สถานะ:** กำลังทำ. ปรับ README/architecture และเพิ่ม CSV ตัวอย่าง; สำเนา source-only ที่ไม่มี `build/` ผ่าน Windows build/test และ CLI train/predict. Benchmark อ้างอิงรายงาน Phase H. CI configuration มีอยู่ แต่ยังยืนยัน hosted run ไม่ได้; workspace ไม่มี Git metadata จึงตรวจ actual clean checkout และสถานะ tracked artifacts ไม่ได้. ดู [Phase I handoff](phase-i-handoff.md) สำหรับรายการตรวจที่เหลือ.
