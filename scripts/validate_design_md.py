#!/usr/bin/env python3
"""
Kinetix L2 — Automated DESIGN.md, WCAG Contrast & Visual-Quality Auditor
Implements Workflows 5, 6, 10, and 12 from `native-cpp-ux-product-design.md`:
1. Validates that DESIGN.md exists, starts with YAML frontmatter containing semantic design tokens,
   and includes the exact 8 ordered Markdown sections:
   Overview, Colors, Typography, Layout, Elevation & Depth, Shapes, Components, Do's and Don'ts.
2. Verifies WCAG 2.2 AA/AAA contrast ratios for primary text on dark and light backgrounds.
3. Verifies that all 10 `references/*.md` guides and `assets/DESIGN.md.template` exist.
"""

import re
import sys
from pathlib import Path

REQUIRED_SECTIONS = [
    "Overview",
    "Colors",
    "Typography",
    "Layout",
    "Elevation & Depth",
    "Shapes",
    "Components",
    "Do's and Don'ts",
]

REQUIRED_TOKEN_PREFIXES = [
    "color.primary",
    "color.background",
    "color.surface",
    "color.text",
    "color.border",
    "color.error",
    "color.success",
    "type.body.fontFamily",
    "type.body.fontSize",
    "spacing.unit",
    "radius.control",
    "radius.panel",
]

REQUIRED_REFERENCES = [
    "assets/DESIGN.md.template",
    "references/persona-methodology.md",
    "references/journey-mapping-guide.md",
    "references/usability-testing-frameworks.md",
    "references/design-system-guidelines.md",
    "references/native-layout-guidelines.md",
    "references/motion-guidelines.md",
    "references/native-accessibility.md",
    "references/native-performance.md",
    "references/design-md-format.md",
    "references/visual-quality-audit.md",
]


def hex_to_rel_luminance(hex_str: str) -> float:
    h = hex_str.strip().strip('"').strip("'").lstrip("#")
    r, g, b = [int(h[i : i + 2], 16) / 255.0 for i in (0, 2, 4)]

    def lin(c: float) -> float:
        return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4

    return 0.2126 * lin(r) + 0.7152 * lin(g) + 0.0722 * lin(b)


def contrast_ratio(hex1: str, hex2: str) -> float:
    l1 = hex_to_rel_luminance(hex1)
    l2 = hex_to_rel_luminance(hex2)
    return (max(l1, l2) + 0.05) / (min(l1, l2) + 0.05)


def main() -> int:
    repo = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(".").resolve()
    design_md = repo / "DESIGN.md"
    errors = []

    if not design_md.exists():
        print("[FAIL] DESIGN.md not found at repository root.")
        return 1

    text = design_md.read_text(encoding="utf-8")
    m = re.match(r"^---\n(.*?)\n---\n(.*)$", text, flags=re.DOTALL)
    if not m:
        print("[FAIL] DESIGN.md missing valid YAML frontmatter block.")
        return 1

    frontmatter, body = m.group(1), m.group(2)
    tokens = {}
    for line in frontmatter.splitlines():
        line = line.strip()
        if not line or line.startswith("#") or ":" not in line:
            continue
        k, v = line.split(":", 1)
        tokens[k.strip()] = v.strip().strip('"').strip("'")

    for req_key in REQUIRED_TOKEN_PREFIXES:
        if req_key not in tokens:
            errors.append(f"Missing required semantic token in DESIGN.md frontmatter: {req_key}")

    headings = re.findall(r"^##\s+(.+?)\s*$", body, flags=re.MULTILINE)
    if headings != REQUIRED_SECTIONS:
        errors.append(f"DESIGN.md sections {headings} do not match required ordered sections {REQUIRED_SECTIONS}")

    if "color.text" in tokens and "color.background" in tokens:
        cr_dark = contrast_ratio(tokens["color.text"], tokens["color.background"])
        if cr_dark < 7.0:
            errors.append(f"Dark theme primary text contrast {cr_dark:.2f}:1 is below WCAG AAA 7.0:1")

    if "color.text.lightMode" in tokens and "color.background.lightMode" in tokens:
        cr_light = contrast_ratio(tokens["color.text.lightMode"], tokens["color.background.lightMode"])
        if cr_light < 7.0:
            errors.append(f"Light theme primary text contrast {cr_light:.2f}:1 is below WCAG AAA 7.0:1")

    for ref_rel in REQUIRED_REFERENCES:
        ref_path = repo / ref_rel
        if not ref_path.exists() or ref_path.stat().st_size < 100:
            errors.append(f"Missing or incomplete UX reference file: {ref_rel}")

    if errors:
        print("=== Native C++ UX & DESIGN.md Audit: FAIL ===")
        for e in errors:
            print(f"  [!] {e}")
        return 1

    print("=== Native C++ UX & DESIGN.md Audit: PASS ===")
    print(f"  [OK] DESIGN.md verified ({len(tokens)} semantic tokens, 8 ordered sections).")
    print(f"  [OK] WCAG AAA contrast verified (Dark={cr_dark:.2f}:1, Light={cr_light:.2f}:1).")
    print(f"  [OK] All {len(REQUIRED_REFERENCES)} UX reference guides & DESIGN.md.template verified.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
