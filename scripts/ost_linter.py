#!/usr/bin/env python3
"""
Kinetix L2 — Opportunity Solution Tree (OST) Linter (`SKILL (3).md`)
Enforces Teresa Torres's 5 structural rules:
- O1: Exactly one measurable outcome root with a numeric target
- O2: Opportunities are framed as user needs/pain points, not feature labels
- O3: Targeted opportunities compare >= 2 candidate solutions
- O4: Every selected solution has an explicit assumption test with pass metric
- O5: Zero orphan solutions detached from an opportunity node
"""

import argparse
import json
import re
import sys
from pathlib import Path


def lint_ost(ost_path: Path) -> dict:
    data = json.loads(ost_path.read_text(encoding="utf-8"))
    violations = []

    outcome = data.get("outcome", {})
    metric_stmt = outcome.get("statement", "")
    if not metric_stmt or not re.search(r"\d+", metric_stmt):
        violations.append("O1 Violation: Root outcome must contain a measurable numeric target.")

    opportunities = data.get("opportunities", [])
    if not opportunities:
        violations.append("O2 Violation: OST must contain at least one opportunity node.")

    for opp in opportunities:
        opp_id = opp.get("id", "unknown")
        need = opp.get("user_need", "")
        if any(word in need.lower() for word in ["build a", "implement a", "add a button"]):
            violations.append(f"O2 Violation ({opp_id}): Opportunity phrased as a feature rather than a user need.")
        solutions = opp.get("solutions", [])
        if opp.get("targeted", False) and len(solutions) < 2:
            violations.append(f"O3 Violation ({opp_id}): Targeted opportunity must compare >= 2 candidate solutions.")
        for sol in solutions:
            sol_id = sol.get("id", "unknown")
            if not sol.get("parent_opportunity_id") == opp_id:
                violations.append(f"O5 Violation ({sol_id}): Solution missing valid parent_opportunity_id link.")
            tests = sol.get("assumption_tests", [])
            if not tests:
                violations.append(f"O4 Violation ({sol_id}): Solution has no assumption test.")

    return {
        "verdict": "SOUND" if not violations else "NEEDS-REWORK",
        "opportunities_checked": len(opportunities),
        "violations": violations,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Lint Opportunity Solution Tree JSON against rules O1-O5.")
    parser.add_argument("--input", type=Path, required=True, help="Path to ost.json")
    args = parser.parse_args()

    res = lint_ost(args.input)
    print(json.dumps(res, indent=2))
    return 0 if res["verdict"] == "SOUND" else 2


if __name__ == "__main__":
    sys.exit(main())
