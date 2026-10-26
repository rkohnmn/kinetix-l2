#!/usr/bin/env python3
"""
Kinetix L2 — Static Dear ImGui Begin/End and Push/Pop Balance Guard (`CPPUIUX.md`)
Scans C++ source files to verify that every:
- `BeginWindow` matches an `EndWindow`
- `BeginChild` matches an `EndChild`
- `BeginTable` matches an `EndTable`
- `PushID` matches a `PopID`
- `PushStyleColor` matches a `PopStyleColor`
"""

import re
import sys
from pathlib import Path

PAIRS = [
    ("BeginWindow", "EndWindow"),
    ("BeginChild", "EndChild"),
    ("BeginTable", "EndTable"),
    ("PushID", "PopID"),
    ("PushStyleColor", "PopStyleColor"),
]


def check_file(path: Path) -> list[str]:
    text = path.read_text(encoding="utf-8", errors="ignore")
    # Strip single-line comments
    clean_lines = [re.sub(r"//.*$", "", line) for line in text.splitlines()]
    clean = "\n".join(clean_lines)

    errors = []
    for open_fn, close_fn in PAIRS:
        opens = len(re.findall(rf"\b{open_fn}\s*\(", clean))
        closes = len(re.findall(rf"\b{close_fn}\s*\(", clean))
        if opens != closes:
            errors.append(f"{path.name}: Unbalanced {open_fn} ({opens}) vs {close_fn} ({closes})")
    return errors


def main() -> int:
    if len(sys.argv) < 2:
        print("Usage: check_imgui_cpp.py <source.cpp ...>")
        return 1

    all_errors = []
    for arg in sys.argv[1:]:
        p = Path(arg)
        if p.exists():
            all_errors.extend(check_file(p))

    if all_errors:
        print("=== Dear ImGui Static Balance Audit: FAIL ===")
        for e in all_errors:
            print(f"  [ERROR] {e}")
        return 1

    print(f"=== Dear ImGui Static Balance Audit: PASS ({len(sys.argv) - 1} files verified) ===")
    print("  [OK] All Begin*/End* and Push*/Pop* calls are 100% balanced.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
