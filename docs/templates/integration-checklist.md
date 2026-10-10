# Integration checklist

- [ ] ตรวจ Git status/diff ก่อนรวม และระบุ unrelated changes
- [ ] Handoff ของ owners ครบ; public contracts ตรงกัน
- [ ] ไม่มี shared-file ownership conflict
- [ ] Build command รันจริงและผลบันทึก
- [ ] Tests รันจริงและผลบันทึก
- [ ] Warnings/errors และ unresolved integration issues ระบุ
- [ ] Security/code review อ่าน integrated diff แล้ว
- [ ] Documentation สะท้อน implementation จริง
- [ ] ไม่มี secrets, binaries หรือ generated files ที่ไม่ตั้งใจ
- [ ] ไม่ commit/push เว้นแต่ได้รับอนุญาต
