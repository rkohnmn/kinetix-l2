# Competitive Teardown & 12-Dimension Scorecard

Following the `competitive-teardown` methodology, this document synthesizes pricing, learner review sentiment, and pedagogical capabilities across six established incumbents (**Duolingo**, **Anki**, **LingQ**, **WaniKani**, **Babbel**, and **Pimsleur**) against **Kinetix L2**.

---

## 1. 12-Dimension Competitive Scorecard (1–5 Scale, Max 60)

| # | Dimension | Kinetix L2 | Anki (FSRS) | LingQ | WaniKani | Babbel | Pimsleur | Duolingo | Evidence & Verification Notes |
|---|-----------|:---:|:---:|:---:|:---:|:---:|:---:|:---:|---|
| 1 | **SLA Pedagogical Depth** | **5** | 3 | 3 | 3 | 4 | 3 | 2 | Kinetix unifies 5 SLA stages & 4 modalities; [Duolingo relies on word banks](https://www.reddit.com/r/duolingo/comments/1qu0ylk/unpopular_opinion_duolingo_isnt_that_bad_and/); [LingQ is purely receptive](https://forum.lingq.com/t/lingq-the-illusion-of-learning/975663). |
| 2 | **Pricing & Value** | **5** | 5 | 3 | 3 | 3 | 2 | 3 | Kinetix core is free MIT local-first; [LingQ raised price to $14.99/mo](https://linguaverbum.com/blog/best-lingq-alternatives-2026/); [Anki iOS is $24.99 one-time](https://www.mindomax.com/is-anki-free). |
| 3 | **UX & Low Friction** | **5** | 2 | 3 | 4 | 4 | 4 | 5 | [Anki has steep card-creation friction](https://nebulearn.app/blog/best-free-anki-alternatives); Kinetix pairs 1-click mining with zero-config FSRS-4.5. |
| 4 | **Native Performance** | **5** | 3 | 2 | 3 | 3 | 4 | 4 | Kinetix is 100% native C++20/SDL2/Cairo with zero WebView; Anki desktop wraps webviews for cards. |
| 5 | **Documentation & Science** | **5** | 4 | 3 | 4 | 3 | 3 | 3 | Full mathematical & psycholinguistic transparency (Baddeley, Ullman, FSRS-4.5 DSR equations). |
| 6 | **Leech & Error Recovery** | **5** | 2 | 1 | 2 | 2 | 2 | 2 | Kinetix auto-triggers 3-step morphological/minimal-pair remediation when lapses > 4 or D > 8.5. |
| 7 | **Cross-Modal Integration** | **5** | 2 | 3 | 2 | 3 | 2 | 3 | Kinetix connects interlinear reading directly to prerequisite-gated active recall and recasts. |
| 8 | **Privacy & Local-First** | **5** | 5 | 2 | 2 | 2 | 2 | 1 | 100% offline SQLite3 WAL storage with zero telemetry; competitors mandate cloud accounts. |
| 9 | **Cross-Platform Parity** | **5** | 4 | 3 | 2 | 3 | 3 | 4 | Native C++20 core targets Windows, macOS, Linux, iOS, and Android with responsive layouts. |
| 10 | **Ethical Retention** | **5** | 4 | 3 | 3 | 4 | 4 | 2 | Kinetix includes Resilient Vacation Shift & Regret-Test stopping cues vs. punitive streak anxiety. |
| 11 | **Community & Openness** | **4** | 5 | 4 | 4 | 2 | 2 | 4 | Open MIT repository, SQLite3 schema, and deterministic JSON CRDT `revlog` format. |
| 12 | **Algorithmic Innovation** | **5** | 5 | 2 | 2 | 3 | 3 | 4 | Native 4-modality FSRS-4.5 + DeKeyser `<2500ms` latency gating + WaniKani dependency DAG. |
| | **TOTAL SCORE (/60)** | **59** | **44** | **32** | **34** | **36** | **34** | **37** | |

---

## 2. Competitor Pricing Breakdown (Verified 2025–2026)

| Platform | Pricing Model | Entry / Monthly | Annual / Lifetime | Key Limitation Identified in User Reviews |
|---|---|---|---|---|
| **Duolingo** | Freemium + [Super / Max Tiers](https://www.reddit.com/r/duolingo/comments/1ilmk5e/duolingos_pricing_tiers_is_it_becoming_too/) | Free (ad/heart-limited) / Super ~$12.99/mo | Super ~$84/yr; Max ~$168/yr | Over-reliance on recognition word banks; punitive streak anxiety; restrictive free tier. |
| **Anki** | Open-Source + [Paid iOS App](https://flica.app/article/is-anki-free) | $0 (Desktop/Android) | [$24.99 one-time on iOS](https://www.mindomax.com/is-anki-free) | Context-free isolation; HTML/WebView card rendering; high setup and deck-maintenance time cost. |
| **LingQ** | Freemium + [Premium Subscription](https://www.lingq.com/blog/lingq-free-vs-premium/) | [$14.99/mo](https://linguaverbum.com/blog/best-lingq-alternatives-2026/) | ~$107.88/yr | [Purely receptive](https://forum.lingq.com/t/lingq-the-illusion-of-learning/975663); weak SRS scheduling; does not train timed syntactic or spoken output. |
| **WaniKani** | Freemium (Levels 1–3) + [Subscription](https://wanilog.com/guides/is-wanikani-worth-it) | [$9.00/mo](https://community.wanikani.com/t/wanikani-pricing-changes/19148) | $89/yr or $299 Lifetime | Japanese Kanji only; fixed SRS intervals rather than FSRS; detached from sentence production. |
| **Babbel / Pimsleur** | Subscription ([Comparison](https://copycatcafe.com/blog/pimsleur-vs-babbel)) | ~$14.95–$20.95/mo | ~$89–$164/yr | Linear syllabus; [Pimsleur audio-only courses lack reading literacy and have low lexical breadth](https://copycatcafe.com/blog/pimsleur-vs-babbel). |
| **Kinetix L2** | **Local-First Free Core + Optional Sync/Packs** | **$0 Core Forever** (Optional $6/mo Sync) | **$49/yr or $129 Lifetime** | Unifies Interlinear Input, 4-Modality FSRS-4.5, WaniKani Dependency Gating, and Timed Output. |

---

## 3. Positioning Map (Pedagogical Depth vs. Frictionless UX)

```
                     HIGH PEDAGOGICAL EFFICACY (Procedural Fluency & FSRS Math)
                                         ^
                                         |
            [Anki + FSRS + Manual Decks] |          ★ [KINETIX L2]
            (High setup friction,        |          (4-Modality FSRS-4.5, 1-Click i+1 Mining,
             isolated cards)             |           Prerequisite DAG, <2.5s Output Studio)
                                         |
                     [WaniKani]          |          [Pimsleur] / [Babbel]
                                         |
HIGH FRICTION / -------------------------+------------------------- LOW FRICTION /
MANUAL SETUP                             |                          GUIDED HABIT LOOP
                                         |
                     [LingQ]             |
                     (Cluttered UI,      |          [Duolingo]
                      receptive only)    |          (Frictionless habit, but trapped
                                         |           in Tier-0 word-bank recognition)
                                         v
                     LOW PEDAGOGICAL EFFICACY (Passive Recognition Only)
```
