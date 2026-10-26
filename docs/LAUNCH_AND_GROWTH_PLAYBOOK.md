# Product Marketing, Positioning, and Launch Playbook

This launch package establishes a realistic, zero-hype distribution strategy for **Kinetix L2** grounded in the verified frustrations of intermediate language learners and spaced-repetition power users.

---

## 1. Product Identity & Narrative

- **Product Name**: **Kinetix L2**
- **Rationale**: Combines *kinetic* procedural execution (Ullman's Declarative/Procedural model & DeKeyser's Skill Acquisition Theory) with second-language (`L2`) mastery.
- **Tagline**: *Stop recognizing words. Start producing language under 2.5 seconds.*
- **Short Description (GitHub / App Store)**:
  Native C++20 cross-platform language acquisition workbench combining interlinear `i+1` reading, 4-modality FSRS-4.5 scheduling, WaniKani-style prerequisite gating, and timed `<2.5s` spoken/written output.
- **5-Question Product Narrative**:
  1. **What is this product?** A blazing-fast, local-first native desktop and mobile application (`C++20` / `SDL2` / `Cairo` / `SQLite3`) that unifies comprehensible input, mathematical spaced repetition (FSRS-4.5), and timed syntactic production.
  2. **Who is it for?** Self-directed learners (A2–B2 / JLPT N4–N2) who are tired of tapping word banks in casual apps or spending hours maintaining fragmented Anki + LingQ + WaniKani pipelines.
  3. **What can users accomplish with it?** Read high-interest dialogues with token-level familiarity heatmaps, mine `i+1` sentences in one click, practice Pimsleur-style prosodic back-chaining, and automate spoken and written grammar under realistic `<2.5s` conversational latency constraints.
  4. **Why is it better suited than alternatives?** It tracks four separate FSRS memory curves per item (`Receptive-Text`, `Receptive-Audio`, `Productive-Text`, `Productive-Audio`), blocks premature production until receptive stability reaches `S >= 3.0 days`, eliminates word-bank crutches after Tier 0, and replaces punitive streaks with a Resilient Vacation Shift.
  5. **Why try it now?** It launches in under 100 milliseconds with zero sign-up wall, zero cloud lock-in, and pre-seeded German, Spanish, and Japanese dependency graphs ready to explore immediately.

---

## 2. GitHub Repository Metadata & Launch Channels

- **GitHub Description**: `Kinetix L2: Native C++20 Cross-Platform 4-Modality Language Acquisition Workbench (FSRS-4.5, Interlinear i+1 Reader, Dependency DAG & <2.5s Procedural Studio — Zero WebView/Electron)`
- **GitHub Topics**: `cpp20`, `language-learning`, `fsrs`, `spaced-repetition`, `sdl2`, `cairo`, `sqlite3`, `second-language-acquisition`, `native-gui`, `cross-platform`, `local-first`
- **Primary Organic Distribution Channels**:
  1. **Reddit Communities**: `r/languagelearning`, `r/Anki`, `r/German`, `r/LearnJapanese`, `r/Spanish` — sharing technical deep-dives on why single-modality flashcards fail productive fluency and how 4-modality FSRS-4.5 + `<2500ms` latency gating works.
  2. **Hacker News (`Show HN`)**: *Show HN: Kinetix L2 – A 3MB native C++20/Cairo language workbench (zero Electron/WebView) with 4-modality FSRS-4.5*.
  3. **Open-Spaced-Repetition & Local-First Communities**: Contributing empirical 4-modality latency benchmarks to the FSRS open-source ecosystem.

---

## 3. Launch Announcement Draft (Show HN / Community Post)

> **Title**: Show HN: Kinetix L2 — Native C++20 4-Modality Language Workbench (FSRS-4.5, Interlinear Reader, Zero WebView)
>
> After hitting the classic intermediate plateau—where I could recognize words on flashcards or tap word banks effortlessly, but froze whenever I had to construct a German `weil` clause or Japanese obligation sentence in real time—I started looking into the psycholinguistics of why most language apps plateau at A2.
>
> Three findings stood out from Second Language Acquisition (SLA) research:
> 1. **Recognition != Production**: Passive recognition uses fast declarative familiarity, while real-time conversation requires implicit procedural execution within `<2.5 seconds` (Ullman's DP Model & DeKeyser's Skill Acquisition Theory).
> 2. **Single-Modality SRS is Flawed**: Knowing a word when reading does not mean you can parse it in fast audio or produce it under time pressure.
> 3. **Premature Output Causes Leech Hell**: Testing productive recall before receptive stability is consolidated (`S_rec >= 3.0 days`) overwhelms working memory.
>
> So we built **Kinetix L2**, an open-source (MIT), local-first native application written in C++20 using SDL2, Cairo 2D vector graphics, and SQLite3 WAL (zero Electron, zero WebView, sub-100ms cold start).
>
> **What's inside v1.0:**
> - **4-Modality FSRS-4.5 Engine**: Tracks `Receptive-Text`, `Receptive-Audio`, `Productive-Text`, and `Productive-Audio` independently for every phoneme, lemma, chunk, and grammar rule.
> - **WaniKani-Style Dependency Gating**: Productive drills remain locked until all prerequisite nodes reach `S_rec >= 3.0 days`.
> - **Stage 1 Interlinear Reader + 1-Click Mining**: Color-codes token familiarity (0–4) and computes exact `i+1` comprehension density (95–98% sweet spot).
> - **Pimsleur-Style Prosodic Back-Chaining & `<2500ms` Output Studio**: Visualizes native F0 pitch contours (`-digung` -> `-schuldigung` -> `Entschuldigung`) and caps FSRS ratings at `Hard` if production takes longer than 2.5 seconds.
> - **Anti-Leech Intervention & Resilient Vacation Shift**: Automatically decomposes leeches (`lapses > 4` or `D > 8.5`) into morphemes and minimal pairs, and lets you shift FSRS timestamps during travel/illness without punitive streak resets.
>
> The full C++20 source, SQLite3 schema, and test suite are in the repository. Feedback on the 4-modality FSRS weighting and latency thresholds is very welcome!

---

## 4. Monetization & Pricing Experiments

- **Experiment 1 (Local-First Free Core vs. $49/yr Sync + Acoustic Packs)**: Measure conversion rate among multi-device users (Desktop Workbench + Mobile Commute) who export/sync their CRDT `revlog` across 2+ operating systems.
- **Experiment 2 ($129 Lifetime License Option)**: Following [WaniKani's proven lifetime tier model](https://wanilog.com/guides/is-wanikani-worth-it), offer a one-time $129 lifetime license to capture subscription-fatigued power users migrating from $14.99/mo subscriptions.
