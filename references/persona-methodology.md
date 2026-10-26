# Workflow 1 & 4: Research-Backed User Personas & Evidence Coding (`kinetix L2`)

## 1. Evidence Coding Taxonomy
All learner research records in `include/kinetix/ui/ux_design_system.hpp` (`ux::ResearchRecord`, `ux::EvidenceTag`) are coded with explicit tags:
- `[GOAL]` — Desired procedural fluency outcome (e.g., passing CEFR B1/B2 or JLPT N4 oral production under `< 2,500 ms` latency).
- `[PAIN]` — Observed friction (e.g., Anki 500-card post-vacation backlog walls; Duolingo recognition word-bank plateau).
- `[BEHAVIOR]` — Observed action (e.g., mining contextual sentences from Stage 1 dialogues into Stage 3 active recall).
- `[CONTEXT]` — Operating environment (Linux/macOS/Windows desktop study vs. hands-free mobile commute).
- `[ACCESSIBILITY]` — Keyboard-only `[Tab]` focus navigation, WCAG AAA contrast (`> 14.9:1`), and `Reduced Motion` (`ux::MotionPreference::ReducedMotion`).

## 2. Validated Behavioral Archetypes
1. **Professional Operator (`BehavioralArchetype::ProfessionalOperator`)**: Daily 15-minute structured sessions targeting German B1 or Japanese N4; relies on the **Consumer Focus Studio** and **5-Minute Regret Test** stopping cue.
2. **Technical Specialist / Power User (`BehavioralArchetype::TechnicalSpecialist`)**: Enables `Pro Telemetry: ON` to inspect 4-modality FSRS-4.5 power-law decay curves, WaniKani prerequisite gates (`S_rec >= 3.0d`), and CRDT `revlog` UUID streams.
3. **Occasional / Returning Learner (`BehavioralArchetype::OccasionalUser`)**: Uses **Resilient Vacation Shift (`+3d`)** to shift SQLite3 review timestamps forward without punitive streak resets.
