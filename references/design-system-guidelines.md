# Workflow 5 & 9: Semantic Design Tokens & Component System (`kinetix L2`)

## 1. Single Source of Truth in C++20
- `include/kinetix/ui/design_tokens.hpp`: Defines `ThemePalette::dark_obsidian()` (Deep Editorial Slate-Teal `#0D181B`, Warm Archival Ivory `#F4EFE6`, Kinetic Mint `#91D1B5`, Terracotta Coral `#EE9A7C`) and `ThemePalette::light_archival()` (Warm Archival Ivory `#F5F0E6`, Deep Slate-Teal `#16272B`, Deep Kinetic Emerald `#1C7B5F`, Terracotta Rust `#B6573D`).
- `include/kinetix/ui/ux_design_system.hpp`: Defines `ux::ControlState` (`Default`, `Hover`, `Pressed`, `Focused`, `Disabled`, `Error`), `ux::MotionPreference` (`StandardSpring`, `ReducedMotion`), and `ux::compute_wcag_contrast_ratio`.

## 2. Non-Color-Alone Status Communication
Every status badge in `src/ui/vector_canvas.cpp` and `src/ui/main_window.cpp` combines semantic color with an explicit symbol prefix:
- `[✓ PROD UNLOCKED]` / `[✓ UNLOCKED]` — Prerequisite receptive stability `S_rec >= 3.0d` satisfied.
- `[⏳ REC GATE <3d]` / `[⏳ GATED]` — Productive output locked until receptive prerequisite reaches `3.0d`.
- `[! LEECH ACTIVE]` — Node has `lapses >= 4` and `D >= 8.2`, triggering contrastive minimal-pair remediation.
