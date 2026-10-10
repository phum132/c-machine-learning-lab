# แนวปฏิบัติการเขียนโค้ด

## C และ API

- เขียนตาม C11; ใช้ `snake_case` กับ function/variable และ prefix `ml_` สำหรับ shared library API
- ประกาศ public interfaces ใน `include/`; implementation ใน `src/`; ใส่ include guard และ include เฉพาะที่ใช้
- ใช้ `const` กับข้อมูลที่ไม่แก้ไข; ให้ชนิดและชื่อบอกหน่วย/มิติเมื่อช่วยลดความสับสน
- ส่งสถานะกลับด้วย `MLStatus` สำหรับ library operation; API ต้องอธิบาย ownership, lifetime, mutability และ output เมื่อเกิด error
- หลีกเลี่ยง global mutable state และ compiler-specific extensions

## หน่วยความจำและตัวเลข

- ตรวจ null, lengths, dimensions และ overflow ก่อน indexing/allocation
- `size_t` ใช้แทนขนาดและ index; ตรวจ overflow ก่อนคูณจำนวน element กับ `sizeof(element)`
- ระบุเจ้าของ allocation ใน API; cleanup ต้องทำได้ทุกเส้นทางล้มเหลว
- ตรวจค่า NaN/Infinity ตาม contract และเลือกสูตรที่ไม่เพิ่ม numerical instability โดยไม่จำเป็น

## รูปแบบและ warnings

- ใช้ `.editorconfig` และ `.clang-format`; ไม่รัน formatter กับไฟล์นอก scope
- โปรไฟล์เริ่มต้น: `-std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow`
- แก้ warning ใน code เมื่อทำได้; suppression ต้องแคบและมีเหตุผลใน code/review

## Tests

- ใช้ expected values ที่เป็นอิสระจาก implementation และ test success, boundary, invalid input, regression ตามความเสี่ยง
- Tests ต้อง deterministic; ไม่ลด assertion เพื่อให้ผ่าน
- test infrastructure ระยะเริ่มต้นใช้ C standard library และ Python standard library เท่านั้น

## Build commands

- POSIX/Git Bash ที่มี Make: `make` หรือ `make test`
- Windows Command Prompt/PowerShell ที่มี GCC: `build.bat` หรือ `build.bat test`
- `CC` ใช้ override compiler ใน Make; `CC_OVERRIDE` ใช้ override ใน batch script
- ใช้ C standard library เท่านั้นใน Foundation; ยังไม่ต้อง link `libm`
