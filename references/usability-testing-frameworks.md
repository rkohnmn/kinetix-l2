# Workflow 3: Usability, Performance & Accessibility Testing Plan (`kinetix L2`)

## 1. Testable Research Questions & Acceptance Targets
| Research Dimension | Testable Question | Verification Method in Repository |
|---|---|---|
| **Task Completion** | Can learners mine an `i+1` token in Stage 1 (`F2`) and complete a speed-gated recall turn in Stage 3 (`F3`) without assistance? | Automated integration test `24_interactive_exercise_submission_and_eval` in `tests/test_main.cpp`. |
| **Error & Filter Recovery** | When a search filter matches 0 nodes, can the user understand why and recover in 1 click or keystroke (`[Esc]`)? | Automated test `30_empty_state_search_filter_and_recovery` in `tests/test_main.cpp`. |
| **Accessibility** | Do all foreground/background text pairings meet WCAG AA (`>= 4.5:1`) and AAA (`>= 7.0:1`), and does `Reduced Motion` eliminate spring oscillation? | Automated tests `28_wcag_contrast_ratios_verified` and `29_reduced_motion_accessibility_preference` in `tests/test_main.cpp`. |
| **Performance** | Does the 44-tuple `ModalityStatesSoA` batch FSRS evaluation complete in `< 50 us` off the UI bottleneck? | Benchmarked at `~7.1 us` (`03_soa_batch_fsrs_evaluation`). |
