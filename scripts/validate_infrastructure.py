"""Validate the repository's agent, skill, and documentation scaffolding."""

from __future__ import annotations

import re
import sys
import tomllib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EXPECTED_ROLES = {
    "architect",
    "c_engineer",
    "matrix_engineer",
    "ml_engineer",
    "data_engineer",
    "testing_engineer",
    "security_reviewer",
    "code_reviewer",
    "documentation_engineer",
    "integration_engineer",
}
EXPECTED_SKILLS = {
    "c-development",
    "memory-safety",
    "matrix-library",
    "ml-from-scratch",
    "csv-parsing",
    "unit-testing",
    "code-review",
    "numerical-stability",
    "benchmarking",
    "documentation",
    "git-workflow",
    "release-checklist",
}
EXPECTED_TEMPLATES = {
    "task-assignment.md",
    "agent-handoff.md",
    "code-review-report.md",
    "test-report.md",
    "benchmark-report.md",
    "architecture-decision.md",
    "integration-checklist.md",
    "release-checklist.md",
}


def fail(message: str) -> None:
    print(f"FAIL: {message}", file=sys.stderr)
    raise SystemExit(1)


def main() -> None:
    config_path = ROOT / ".codex" / "config.toml"
    try:
        config = tomllib.loads(config_path.read_text(encoding="utf-8"))
    except (OSError, tomllib.TOMLDecodeError) as exc:
        fail(f"อ่าน .codex/config.toml ไม่ได้: {exc}")

    agents = config.get("agents", {})
    roles = {key for key, value in agents.items() if isinstance(value, dict)}
    if roles != EXPECTED_ROLES:
        fail(f"role ไม่ตรงรายการที่กำหนด: พบ {sorted(roles)}")

    for role in sorted(EXPECTED_ROLES):
        entry = agents[role]
        if not entry.get("description"):
            fail(f"role {role} ไม่มี description")
        config_file = entry.get("config_file")
        if not config_file:
            fail(f"role {role} ไม่มี config_file")
        role_path = ROOT / ".codex" / config_file
        try:
            role_config = tomllib.loads(role_path.read_text(encoding="utf-8"))
        except (OSError, tomllib.TOMLDecodeError) as exc:
            fail(f"role config {role_path.relative_to(ROOT)} ไม่ถูกต้อง: {exc}")
        if not role_config.get("developer_instructions", "").strip():
            fail(f"role {role} ไม่มี developer_instructions")
        instructions = role_config["developer_instructions"]
        for required in ("หน้าที่:", "ขอบเขต:", "ไฟล์:", "การพึ่งพา:", "ตรวจสอบ:", "ส่งมอบ:"):
            if required not in instructions:
                fail(f"role {role} ไม่มีหัวข้อ {required}")

    skills_root = ROOT / ".agents" / "skills"
    skill_dirs = {item.name for item in skills_root.iterdir() if item.is_dir()}
    if skill_dirs != EXPECTED_SKILLS:
        fail(f"skill directories ไม่ตรงรายการ: พบ {sorted(skill_dirs)}")
    for skill in sorted(EXPECTED_SKILLS):
        path = skills_root / skill / "SKILL.md"
        try:
            content = path.read_text(encoding="utf-8")
        except OSError as exc:
            fail(f"อ่าน {path.relative_to(ROOT)} ไม่ได้: {exc}")
        match = re.match(r"\A---\n(.*?)\n---\n", content, re.DOTALL)
        if not match:
            fail(f"{path.relative_to(ROOT)} ไม่มี YAML frontmatter ที่ปิดครบ")
        fields = dict(
            re.findall(r"^(name|description):\s*(.*?)\s*$", match.group(1), re.MULTILINE)
        )
        if fields.get("name") != skill or not fields.get("description"):
            fail(f"frontmatter name/description ไม่ถูกต้องใน {path.relative_to(ROOT)}")
        for heading in ("## ขั้นตอน", "## ตรวจคุณภาพ", "## พลาดบ่อย", "## เกณฑ์รับ"):
            if heading not in content:
                fail(f"{path.relative_to(ROOT)} ไม่มีหัวข้อ {heading}")

    templates_root = ROOT / "docs" / "templates"
    templates = {path.name for path in templates_root.glob("*.md")}
    if templates != EXPECTED_TEMPLATES:
        fail(f"templates ไม่ตรงรายการ: พบ {sorted(templates)}")

    required_docs = (
        "AGENTS.md",
        "README.md",
        "docs/agent-workflow.md",
        "docs/architecture.md",
        "docs/coding-conventions.md",
        "docs/implementation-roadmap.md",
        "docs/audit-report.md",
        "Makefile",
        "build.bat",
        "scripts/install_windows_toolchain.cmd",
        "main.c",
        "include/ml_error.h",
        "src/ml_error.c",
        "tests/test_main.c",
        ".github/workflows/ci.yml",
    )
    missing = [relative for relative in required_docs if not (ROOT / relative).is_file()]
    if missing:
        fail(f"เอกสารที่จำเป็นหายไป: {missing}")

    print(
        "PASS: TOML ของ Codex roles ถูกต้อง; "
        f"พบ roles {len(roles)}, skills {len(skill_dirs)}, templates {len(templates)} "
        "และเอกสารหลักครบ"
    )


if __name__ == "__main__":
    main()
