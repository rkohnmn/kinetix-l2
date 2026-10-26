# Workflow 6: `DESIGN.md` Specification Format & Automated Validation

## 1. Required Structure
1. **YAML Frontmatter**: Literal semantic tokens (`color.*`, `type.*`, `spacing.*`, `size.*`, `radius.*`, `border.*`, `elevation.*`, `motion.*`, `focus.*`, `contrast.*`, `text.*`).
2. **Eight Ordered Markdown Sections**:
   - `## Overview`
   - `## Colors`
   - `## Typography`
   - `## Layout`
   - `## Elevation & Depth`
   - `## Shapes`
   - `## Components`
   - `## Do's and Don'ts`

## 2. Automated Verification
Run `python3 scripts/validate_design_md.py .` (included in `make audit`) to validate YAML token completeness, section ordering, and WCAG contrast ratios.
