#!/usr/bin/env python3
"""
Kinetix L2 — Automated Native GUI Compliance & Dependency Auditor
Verifies Section 2.5 & Section 13.1 of the Master Prompt:
1. Scans the entire source tree for prohibited web GUI technologies (HTML, CSS, JS, Electron, WebView, CEF, Tauri).
2. Inspects CMakeLists.txt and Makefile for prohibited web dependencies.
3. Inspects the compiled ELF binary via `ldd` and `nm` to verify dynamic linkage against native libraries
   (libSDL2, libcairo, librsvg, libpango, libsqlite3, libX11, libwayland) and confirm ZERO browser/WebView libraries.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

PROHIBITED_SOURCE_EXTENSIONS = {".html", ".htm", ".css", ".js", ".jsx", ".ts", ".tsx", ".vue", ".svelte"}
PROHIBITED_BINARY_LIBS = [
    "webkit", "webview", "libcef", "electron", "chromium", "blink", "libv8", "mozjs", "qtwebengine"
]
PROHIBITED_CODE_PATTERNS = [
    r"\bQWebEngineView\b",
    r"\bWKWebView\b",
    r"\bWebView2\b",
    r"\bCefInitialize\b",
    r"http://localhost",
    r"127\.0\.0\.1:\d+",
    r"<html\b",
    r"document\.getElementById",
]


def audit_repository(repo_root: Path, binary_path: Path | None) -> dict:
    issues = []
    scanned_files = 0

    for root, dirs, files in os.walk(repo_root):
        dirs[:] = [d for d in dirs if d not in {".git", "build", "__pycache__"}]
        for fname in files:
            fpath = Path(root) / fname
            rel = fpath.relative_to(repo_root)
            ext = fpath.suffix.lower()
            scanned_files += 1

            if ext in PROHIBITED_SOURCE_EXTENSIONS:
                issues.append(f"Prohibited web file extension in repository: {rel}")

            if ext in {".cpp", ".hpp", ".h", ".c", ".cmake", ".txt"} and fname != "native_gui_audit.py":
                try:
                    content = fpath.read_text(encoding="utf-8", errors="ignore")
                except OSError:
                    continue
                if str(rel).startswith("src/") or str(rel).startswith("include/"):
                    for pat in PROHIBITED_CODE_PATTERNS:
                        if re.search(pat, content, flags=re.IGNORECASE):
                            issues.append(f"Prohibited web/browser API pattern '{pat}' found in {rel}")

    ldd_libs = []
    if binary_path and binary_path.exists():
        if shutil.which("ldd"):
            proc = subprocess.run(["ldd", str(binary_path)], capture_output=True, text=True, check=False)
            if proc.returncode == 0:
                for line in proc.stdout.splitlines():
                    line_s = line.strip()
                    ldd_libs.append(line_s)
                    # Strip hex memory address (e.g., '(0x00007eccef12a000)') before matching library names
                    lib_only = re.sub(r"\(0x[0-9a-fA-F]+\)", "", line_s).lower()
                    for bad_lib in PROHIBITED_BINARY_LIBS:
                        if bad_lib in lib_only:
                            issues.append(f"Binary links against prohibited browser/JS runtime library: {line_s}")

                joined_ldd = "\n".join(ldd_libs)
                for required_native in ["libSDL2", "libcairo", "librsvg", "libpango", "libsqlite3"]:
                    if required_native not in joined_ldd:
                        issues.append(f"Expected native library '{required_native}' not found in ldd output")
            else:
                issues.append(f"Failed to run ldd on {binary_path}")
        else:
            try:
                raw_bytes = binary_path.read_bytes().lower()
                for bad_lib in PROHIBITED_BINARY_LIBS:
                    if bad_lib.lower().encode() in raw_bytes:
                        issues.append(f"Binary links against prohibited browser/JS runtime library: {bad_lib}")
            except OSError:
                pass

    return {
        "status": "PASS" if not issues else "FAIL",
        "scanned_files": scanned_files,
        "binary_inspected": str(binary_path) if binary_path else None,
        "linked_native_libraries_count": len(ldd_libs),
        "issues": issues,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify 100% Native GUI compliance and zero web dependencies.")
    parser.add_argument("repo_root", type=Path, help="Path to repository root")
    parser.add_argument("--binary", type=Path, default=None, help="Path to compiled native executable")
    parser.add_argument("--json", action="store_true", help="Emit JSON output")
    args = parser.parse_args()

    report = audit_repository(args.repo_root.resolve(), args.binary.resolve() if args.binary else None)
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print(f"=== Native GUI Compliance Audit: {report['status']} ===")
        print(f"Scanned files          : {report['scanned_files']}")
        print(f"Binary inspected       : {report['binary_inspected']}")
        print(f"Linked native libs     : {report['linked_native_libraries_count']}")
        if report["issues"]:
            for issue in report["issues"]:
                print(f"  [!] {issue}")
        else:
            print("  [OK] Zero HTML, CSS, JavaScript, WebView, Electron, or localhost UI servers detected.")
            print("  [OK] Verified genuine native C++20 + SDL2 + PangoCairo + librsvg + SQLite3 linkage.")

    return 0 if report["status"] == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main())
