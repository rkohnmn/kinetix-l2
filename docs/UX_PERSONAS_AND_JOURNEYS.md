# UX Research, Data-Driven Personas, Journey Maps, and Ethical Habit Engineering

Built using the `ux-researcher-designer` (`SKILL (5).md`) and `the_anatomy_of_addictive_apps.md` frameworks, this document defines our target user archetypes, end-to-end journey maps, usability testing protocol, and ethical behavioral architecture.

---

## 1. Data-Driven User Personas

### Persona 1: Maya — The "Intermediate Plateau" Breaker (Primary Persona, 42% of Target Segment)
- **Demographics & Context**: 29-year-old software product manager relocating to Munich; uses macOS/Linux at her desk and iOS on the U-Bahn commute.
- **Current Behavior**: Maintained a 650-day streak on a gamified mobile app and completed A2 German, yet froze when a Deutsche Bahn conductor explained a platform change at München Hauptbahnhof.
- **Core Goals**:
  1. Understand rapid native spoken announcements (`Receptive-Audio`) without relying on subtitles.
  2. Produce subordinate clauses (`weil ... Verb-End`) and polite pragmatic chunks (`Entschuldigung`, `Bescheid sagen`) within `<2.5 seconds` (`Productive-Audio` & `Productive-Text`).
- **Validated Frustrations**:
  - Word banks let her guess answers via English word order without encoding German verb-final syntax.
  - Missing two days during a work sprint reset her streak and flooded her with guilt notifications.
- **Design Implications in Kinetix L2**:
  - Restrict recognition choices strictly to Tier 0 ($S < 1\text{ day}$); require zero-word-bank active recall for Tiers 1–3.
  - Provide Pimsleur-style prosodic back-chaining (`-digung` $\rightarrow$ `-schuldigung` $\rightarrow$ `Entschuldigung`) with visual pitch contours.

### Persona 2: Kenji — The "Fragmented Toolchain" Power Learner (Secondary Persona, 34% of Target Segment)
- **Demographics & Context**: 34-year-old data engineer studying Spanish (B1) and Japanese (N3) on Linux and Android.
- **Current Behavior**: Juggles LingQ for reading, Anki (FSRS) for vocabulary, and WaniKani for Kanji radicals.
- **Core Goals**:
  1. Read authentic dialogues at 95–98% known-word density and mine `i+1` sentences into FSRS in a single click.
  2. Prevent "leech hell" when similar words (`Entschuldigung` vs. `Entscheidung`) repeatedly fail recall.
- **Design Implications in Kinetix L2**:
  - Native 1-click sentence mining from Screen 2 (Interlinear Reader) directly into Screen 3 (Active Recall Studio).
  - Automated Anti-Leech Intervention Sub-Routine triggered whenever `lapses > 4` or `D > 8.5`.

---

## 2. End-to-End User Journey Map

| Stage | 1. First-Run Onboarding (<60s) | 2. Stage 1 Input & Noticing (1 min) | 3. Stage 2-4 Active Recall & Output (2 min) | 4. Stage 5 Roleplay & Stopping Cue (1 min) |
|---|---|---|---|---|
| **User Action** | Launches `kinetix-l2`; selects `DE Deutsch (B1)`, `ES Español`, or `JA 日本語`; sees Endowed Progress bar. | Opens Screen 2 (Interlinear Reader); clicks highlighted `i+1` token (`Entschuldigung`); mines context. | Completes suffix back-chaining & types/speaks cloze response under `<2,500 ms` timer (`F3`). | Completes 1-turn graph-constrained recast roleplay (`F5`); reaches "All Caught Up" stopping cue. |
| **Touchpoint** | Screen 1: Cognitive Workbench Overview | Screen 2: Interlinear Reader + Morphology Sidebar | Screen 3: Active Recall & Anti-Leech Studio | Screen 5: Roleplay & Resilient Momentum Panel |
| **Emotional State** | Relieved & Curious (4/5) — zero sign-up wall, instant native launch. | Immersed (5/5) — 96.4% of surrounding words are already known. | Challenged & Focused (4/5) — desirable difficulty without word banks. | Accomplished (5/5) — passes the **5-Minute Regret Test**. |
| **Pain Eliminated** | Eliminates 7-step account creation & credit-card paywalls. | Eliminates external dictionary context-switching. | Eliminates illusion of competence and repetitive leech frustration. | Eliminates doomscrolling and punitive streak anxiety. |

---

## 3. Ethical Behavioral Architecture (Hook Model + $B = MAP$ + Regret Test)

We engineer high daily retention by aligning behavioral mechanics strictly with cognitive consolidation:

1. **B.J. Fogg's Behavior Model ($B = MAP$)**:
   - **Motivation ($M$)**: Driven by visible growth on the FSRS-4.5 power-law forgetting curve ($S \rightarrow S'$) and the 4-Modality Radar polygon.
   - **Ability ($A$)**: Maximized via sub-second native C++20 startup, keyboard hotkeys (`F1–F5`, `Tab`, `Enter`), and a single primary CTA on Screen 1 (`Launch Due Studio`).
   - **Prompt ($P$)**: Internal trigger ("I have 4 minutes before my meeting/train stop") paired with optional, user-controlled local reminders (disabled by default).
2. **Endowed Progress & Zeigarnik Open Loops**:
   - New daily sessions start with 2 foundational cognate/phoneme steps pre-credited on the progress bar (`HabitEngine::compute_endowed_progress_ratio`), accelerating task initiation.
3. **Resilient Momentum vs. Punitive Streaks**:
   - When life interrupts study, clicking **"Activate Resilient Vacation Shift"** on Screen 5 shifts all `modality_states.last_review_ts` forward in SQLite3 so learners never return to a demoralizing 500-card backlog or lose their momentum badge.
4. **The 5-Minute Regret Test Stopping Cue**:
   - Once `completed_tasks_today >= daily_target_tasks` and due items are cleared, Screen 1 displays `"REGRET TEST PASSED: ALL CAUGHT UP"`, advising the learner to rest so hippocampal memory consolidation can occur.
