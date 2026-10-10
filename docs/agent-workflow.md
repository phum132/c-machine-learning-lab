# กระบวนการทำงานหลาย agent

## บทบาทและช่วงงาน

- **วางแผน:** Architect ตรวจ requirement, ระบุ module contracts, dependency, ความเสี่ยง และไฟล์ร่วม
- **มอบหมาย:** Coordinator แบ่งงานเป็น task ขนาดตรวจทานได้ พร้อม owner ต่อไฟล์และ acceptance criteria
- **ลงมือ:** Agents ทำเฉพาะไฟล์ที่ได้รับมอบหมาย งานอิสระทำพร้อมกันได้เมื่อไม่มี dependency หรือ shared-file conflict
- **ตรวจ:** Testing engineer เพิ่มและรัน tests; Security reviewer ตรวจ input/memory/file safety; Code reviewer อ่าน implementation ที่รวมแล้ว
- **รวมงาน:** Integration engineer ตรวจ contract ระหว่างโมดูล รัน build/test และแก้เฉพาะ integration conflict ที่ได้รับมอบหมาย
- **เอกสารและรายงาน:** Documentation engineer ปรับเอกสารตามของจริง; Coordinator สรุปผล ช่องว่าง และความเสี่ยง

การวางแผนกำหนดสิ่งที่จะทำ การมอบหมายกำหนด owner และผลส่งมอบ การลงมือแก้ไฟล์ การตรวจประเมินงานโดยไม่แทนที่เจ้าของ implementation และการรวมทำให้ชิ้นงานเข้ากันเป็นผลเดียว

## ขั้นตอน

1. Architect ตรวจ task และกำหนด contract, dependency และไฟล์ร่วม
2. Coordinator แบ่งงาน bounded tasks โดยทุกงานใช้ [`templates/task-assignment.md`](templates/task-assignment.md)
3. มอบหมายงานอิสระพร้อมกันได้เฉพาะเมื่อระบุไฟล์ owner ชัดและไม่มี dependency ที่ยังไม่ตกลง
4. แต่ละ agent ตรวจสถานะก่อนแก้ ห้ามเขียนไฟล์นอก ownership และรายงานผลจริงตาม [`templates/agent-handoff.md`](templates/agent-handoff.md)
5. แก้ conflict ownership ก่อน implementation; shared headers/public interfaces ต้องตกลงก่อนผู้พึ่งพาเริ่ม
6. Integration engineer รวมงานโดยรักษาการเปลี่ยนแปลงเดิม ตรวจ diff และรัน build/test
7. Testing engineer ตรวจ suite ตาม acceptance; ห้ามลด assertion เพื่อให้ผ่าน
8. Security reviewer และ Code reviewer ตรวจ integrated implementation จริง ไม่พึ่ง summary อย่างเดียว
9. Documentation engineer ปรับ docs ตาม behavior และผลยืนยัน
10. Coordinator ตรวจ acceptance criteria, handoffs และ unresolved risks แล้วรายงานสถานะ

## Concurrency และ ownership

- หนึ่งไฟล์มี write owner เดียวในช่วงเวลาใดเวลาหนึ่ง
- หากไม่มี isolated worktree/workspace ที่ยืนยันว่าใช้งานได้ ให้ serialize งานที่แตะไฟล์เดียวกัน
- Shared header, build config, test harness และ public contract ให้ owner เดียว หรือหยุดรอการตกลงก่อน
- ผู้เขียน test ต้องทราบ expected behavior/API; reviewer ต้องเห็น integrated diff
- ห้ามรวมงานที่ยังไม่ผ่าน verification ที่กำหนด; แยก “ยังไม่รัน” ออกจาก “ผ่าน”
- ห้าม commit/push เว้นแต่เจ้าของโครงการสั่งโดยตรง

## Task ต้องระบุ

Objective, scope, owned files, dependencies, constraints, acceptance criteria, verification commands, expected output และ handoff requirements ต้องครบก่อนเริ่มงาน ดู template สำหรับรูปแบบ

## ข้อจำกัด runtime

ไฟล์ role ใน `.codex/config.toml` เป็น project-scoped role declarations; Codex จะโหลด project config เมื่อ project ถูก trust เท่านั้น การสร้างไฟล์ไม่ได้พิสูจน์ว่า runtime ที่ติดตั้งค้นพบหรือเรียก role ได้ ให้ตรวจด้วย Codex runtime ก่อนใช้งานจริง หากไม่มี subagent tooling ให้ coordinator มอบหมายด้วยมือ และห้ามอ้างว่าได้ delegate จริง
