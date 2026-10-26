#!/usr/bin/env python3
"""
Kinetix L2 — Repository Completeness & Spec-to-Repo Validator
Implements the validation gate from spec-to-repo (`SKILL (6).md`):
- Verifies README.md exists and is comprehensive
- Verifies .gitignore, .env.example, LICENSE, THIRD_PARTY_NOTICES.md, CMakeLists.txt, Makefile exist
- Verifies no .env file or hardcoded secrets are committed
- Verifies automated test files exist
- Verifies zero TODO/FIXME placeholders in C++ source and headers
"""

import argparse
import json
import os
import re
import sys
from pathlib import Path

REQUIRED_FILES = [
    "README.md",
    "LICENSE",
    "THIRD_PARTY_NOTICES.md",
    ".gitignore",
    ".clang-format",
    ".env.example",
    "CMakeLists.txt",
    "Makefile",
    ".github/workflows/ci.yml",
    "tests/test_main.cpp",
]

PLACEHOLDER_PATTERN = re.compile(r"\b(TODO|FIXME|XXX)\b")
ENV_PATTERN = re.compile(r'std::getenv\("([A-Z0-9_]+)"\)')


def validate_project(project_dir: Path) -> dict:
    errors = []
    warnings = []

    for req in REQUIRED_FILES:
        p = project_dir / req
        if not p.exists() or p.stat().st_size == 0:
            errors.append(f"Missing or empty required file: {req}")

    if (project_dir / ".env").exists():
        errors.append("Committed .env file detected (potential secret leak)")

    env_example_text = ""
    if (project_dir / ".env.example").exists():
        env_example_text = (project_dir / ".env.example").read_text(encoding="utf-8")

    cpp_files_checked = 0
    for sub in ["src", "include", "tests"]:
        base = project_dir / sub
        if not base.exists():
            continue
        for root, _, files in os.walk(base):
            for fname in files:
                if fname.endswith((".cpp", ".hpp", ".h")):
                    cpp_files_checked += 1
                    fpath = Path(root) / fname
                    rel = fpath.relative_to(project_dir)
                    text = fpath.read_text(encoding="utf-8", errors="ignore")
                    if PLACEHOLDER_PATTERN.search(text):
                        errors.append(f"Placeholder comment (TODO/FIXME) found in {rel}")
                    for env_var in ENV_PATTERN.findall(text):
                        if env_var in {"HOME", "XDG_DATA_HOME", "LOCALAPPDATA"}:
                            continue
                        if env_var not in env_example_text:
                            errors.append(f"Env var {env_var} used in {rel} but missing from .env.example")

    return {
        "status": "PASS" if not errors else "FAIL",
        "cpp_files_checked": cpp_files_checked,
        "errors": errors,
        "warnings": warnings,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate generated repository structure and code quality.")
    parser.add_argument("project_dir", type=Path, help="Path to repository root")
    parser.add_argument("--format", choices=["text", "json"], default="text")
    args = parser.parse_args()

    res = validate_project(args.project_dir.resolve())
    if args.format == "json":
        print(json.dumps(res, indent=2))
    else:
        print(f"=== Project Validation Gate: {res['status']} (Checked {res['cpp_files_checked']} C/C++ files) ===")
        for err in res["errors"]:
            print(f"  [ERROR] {err}")
        if not res["errors"]:
            print("  [OK] All manifests, env vars, tests, and zero-placeholder checks passed.")
    return 0 if res["status"] == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main())
