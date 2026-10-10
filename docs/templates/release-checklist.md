# Release checklist

- [ ] Acceptance criteria ของระยะ/รุ่นครบ
- [ ] Build และ tests ผ่านจากคำสั่งที่บันทึก
- [ ] Security/code review findings มีการจัดการหรือเปิดเผย
- [ ] Sanitizer/static analysis availability และผลระบุ
- [ ] README/examples/limitations ตรงกับ implementation
- [ ] Benchmark results มี methodology และวัดจริง หรือระบุว่ายังไม่มี
- [ ] ไม่มี secret, database, binary หรือ artifact ที่ไม่ตั้งใจ
- [ ] CI run จริงผ่าน (config อย่างเดียวไม่นับว่าผ่าน)
- [ ] Known issues และ compatibility ระบุ
- [ ] Release/commit/tag ได้รับอนุมัติตาม workflow
