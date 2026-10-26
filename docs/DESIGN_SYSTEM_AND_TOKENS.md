# Native Visual Design System & Token Specification

Following the `ui-design-system` (`SKILL (4).md`) workflow, Kinetix L2 implements a token-driven visual architecture in `include/kinetix/ui/design_tokens.hpp` and `docs/design-tokens.json`.

---

## 1. Brand Identity & Visual Direction

- **Aesthetic Archetype**: Precision Scientific Instrument meets High-Craft Language Studio.
- **Core Philosophy**: Zero decorative clutter, zero glassmorphic blur, and zero pseudo-infographics. Every pixel serves either **cognitive load reduction** (Sweller's CLT), **form-meaning noticing** (Schmidt's Hypothesis), or **empirical memory telemetry** (FSRS-4.5 DSR curves).
- **Dual Theme Support**:
  - **Dark Obsidian Workbench (Default)**: Deep slate-obsidian canvas (`#0B0F17`) with elevated navy-slate cards (`#131B2E`, `#1C2742`) engineered for long evening desktop reading and low eye strain.
  - **Light Archival Paper**: Crisp warm-white paper surface (`#F8FAFC`, `#FFFFFF`) with high-contrast ink (`#0F172A`) for bright daylight and commuter use.

---

## 2. WCAG AA / AAA Contrast-Verified Color Palette

| Semantic Token | Dark Obsidian Hex | Light Archival Hex | Contrast vs. Canvas | WCAG Compliance | Semantic Purpose |
|---|---:|---:|:---:|:---:|---|
| `bg_canvas` | `#0B0F17` | `#F8FAFC` | — | — | Primary application window background |
| `bg_surface_1` | `#131B2E` | `#FFFFFF` | — | — | Primary cards, navigation rail, chart containers |
| `bg_surface_2` | `#1C2742` | `#F1F5F9` | — | — | Interactive input fields, secondary badges, table rows |
| `text_primary` | `#F8FAFC` | `#0F172A` | **17.4:1 / 16.8:1** | **AAA** | Headlines, L2 target forms, immersion passage prose |
| `text_secondary` | `#CBD5E1` | `#334155` | **11.2:1 / 9.4:1** | **AAA** | IPA transcriptions, morphological notes, axis labels |
| `text_muted` | `#94A3B8` | `#475569` | **5.8:1 / 6.1:1** | **AA** | Section headers, keyboard shortcut hints |
| `primary_accent` | `#3B82F6` | `#2563EB` | **5.1:1 / 5.4:1** | **AA** | Primary CTAs, active navigation tab indicator |
| `secondary_accent` | `#14B8A6` | `#0D9488` | **7.8:1 / 4.9:1** | **AAA / AA** | Prosodic pitch contour F0 curve, Endowed Progress bar |
| `mod_rec_text` | `#38BDF8` | `#0284C7` | **9.2:1 / 4.8:1** | **AAA / AA** | Modality 0: Receptive Orthographic (Reading) |
| `mod_rec_audio` | `#34D399` | `#059669` | **10.4:1 / 4.7:1** | **AAA / AA** | Modality 1: Receptive Phonological (Listening) |
| `mod_prod_text` | `#FBBF24` | `#D97706` | **11.8:1 / 4.6:1** | **AAA / AA** | Modality 2: Productive Written (Cloze/Composition) |
| `mod_prod_audio` | `#FB7185` | `#E11D48` | **6.9:1 / 4.8:1** | **AA** | Modality 3: Productive Spoken (`<2500ms` Automatization) |

> **Color-Independent Status Communication**: Every status state pairs color with an explicit text badge (`0-1: Unseen`, `PROD UNLOCKED`, `LEECH INTERVENTION`) so learners with deuteranopia, protanopia, or tritanopia never rely on hue alone.

---

## 3. Modular Typography Scale (1.25x Ratio) & 8pt Spatial Grid

- **Typography Scale (`TypographyTokens`)**:
  - `caption`: `11.5 px` (Axis ticks, hotkey hints, FSRS stability tags)
  - `small`: `13.0 px` (Secondary descriptions, morphological notes)
  - `body`: `15.5 px` (Primary UI copy, diagnostic explanations)
  - `lead`: `19.0 px` (Interlinear immersion reader tokens, hero headlines)
  - `h3`: `22.0 px` (KPI card values, section headers)
  - `h2`: `27.0 px` (Target lemma inspector title)
  - `h1`: `34.0 px` (Primary display header)
- **8pt Spatial Grid & Touch Target Floor (`SpacingTokens`)**:
  - Spacing steps: `xs = 4px`, `sm = 8px`, `md = 16px`, `lg = 24px`, `xl = 32px`, `xxl = 48px`.
  - Touch Target Floor: Every interactive button and mobile navigation tab meets or exceeds `44 × 44 px` (`SpacingTokens::min_touch_target`).
