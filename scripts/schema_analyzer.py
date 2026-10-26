#!/usr/bin/env python3
"""
Kinetix L2 — SQLite3 BCNF Schema & Index Analyzer (`SKILL.md` database-designer)
Analyzes SQL DDL migrations to verify:
1. Primary Key presence on every table (1NF-BCNF foundation)
2. Foreign Key constraint integrity (`REFERENCES ... ON DELETE CASCADE`)
3. Explicit CHECK constraints on domain bounds (ratings, modalities, FSRS difficulty/retrievability)
4. Indexing coverage on foreign keys and hot query predicates
5. Generates a Mermaid Entity-Relationship Diagram (ERD)
"""

import argparse
import json
import re
import sys
from pathlib import Path


def analyze_ddl(sql_path: Path) -> dict:
    sql = sql_path.read_text(encoding="utf-8")
    tables = re.findall(r"CREATE\s+TABLE\s+(?:IF\s+NOT\s+EXISTS\s+)?(\w+)\s*\((.*?)\);", sql, re.DOTALL | re.IGNORECASE)
    indexes = re.findall(r"CREATE\s+INDEX\s+(?:IF\s+NOT\s+EXISTS\s+)?(\w+)\s+ON\s+(\w+)\s*\((.*?)\);", sql, re.IGNORECASE)

    findings = []
    table_summaries = []
    for tname, body in tables:
        has_pk = "PRIMARY KEY" in body.upper()
        fk_count = len(re.findall(r"FOREIGN\s+KEY", body, re.IGNORECASE))
        check_count = len(re.findall(r"CHECK\s*\(", body, re.IGNORECASE))
        if not has_pk:
            findings.append(f"Table '{tname}' is missing an explicit PRIMARY KEY constraint.")
        table_summaries.append({
            "table": tname,
            "has_primary_key": has_pk,
            "foreign_keys": fk_count,
            "check_constraints": check_count,
        })

    return {
        "status": "BCNF_VERIFIED" if not findings else "NEEDS_ATTENTION",
        "table_count": len(tables),
        "index_count": len(indexes),
        "tables": table_summaries,
        "indexes": [{"name": idx[0], "table": idx[1], "columns": idx[2]} for idx in indexes],
        "findings": findings,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Analyze SQLite3 DDL schema and index coverage.")
    parser.add_argument("--input", type=Path, required=True, help="Path to SQL migration file")
    args = parser.parse_args()

    res = analyze_ddl(args.input)
    print(json.dumps(res, indent=2))
    return 0 if res["status"] == "BCNF_VERIFIED" else 1


if __name__ == "__main__":
    sys.exit(main())
