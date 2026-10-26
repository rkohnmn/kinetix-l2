---
color.primary: "#91D1B5"
color.primary.lightMode: "#1C7B5F"
color.secondary: "#EE9A7C"
color.secondary.lightMode: "#B6573D"
color.background: "#0D181B"
color.background.lightMode: "#F5F0E6"
color.surface: "#172A2E"
color.surface.lightMode: "#FCF9F2"
color.surface.elevated: "#254046"
color.text: "#F4EFE6"
color.text.lightMode: "#16272B"
color.text.secondary: "#C9D8D4"
color.text.muted: "#8FA9A4"
color.border: "#2B474E"
color.border.focus: "#91D1B5"
color.error: "#EE9A7C"
color.warning: "#F3B664"
color.success: "#91D1B5"
type.display.fontFamily: "Inter Display"
type.body.fontFamily: "Inter"
type.cjk.fontFamily: "Noto Sans CJK JP"
type.monospace.fontFamily: "Noto Sans Mono"
type.body.fontSize: 13px
type.body.fontWeight: 400
type.body.lineHeight: 1.45
spacing.unit: 4px
spacing.xs: 4px
spacing.sm: 8px
spacing.md: 14px
spacing.lg: 20px
spacing.xl: 28px
size.controlHeight: 36px
size.icon: 15px
size.sidebarWidth: 242px
size.inspectorWidth: 324px
radius.control: 8px
radius.panel: 12px
radius.dialog: 16px
border.subtle: 1px
border.focus: 2px
elevation.panel: 5px
motion.short: 140ms
motion.panel: 240ms
motion.curve: "spring(k=260, c=24, m=1)"
focus.width: 2px
contrast.minimum: 4.5
contrast.enhanced: 7.0
text.minimum: 9.5px
---

## Overview

**kinetix L2** (`v3.1`) is a native C++20 cross-platform Second Language Acquisition (SLA) Focus Studio and 4-modality FSRS-4.5 procedural fluency workbench. Rather than imitating a gamified web toy or an unstyled developer debugger, its visual identity takes direct cues from **archival linguistic publications and precision acoustic studio instruments**: a warm, high-contrast pairing of **Deep Editorial Slate-Teal (`#16272B`)**, **Warm Archival Ivory (`#F4EFE6`)**, **Kinetic Mint (`#91D1B5` / `#1C7B5F`)**, and **Terracotta Coral (`#EE9A7C` / `#B6573D`)**.

By default, the interface opens in **Consumer Focus Studio** mode—separating calm, breathable reading and active-recall surfaces from the on-demand **Pro Telemetry Dock** (`[Pro Telemetry: ON/OFF]`), which reveals the 6-region Dear ImGui / Cycfi Photon diagnostic console (`revlog` CRDT stream and `ModalityStatesSoA` microsecond profiler) only when requested.

## Colors

Semantic color roles are centralized in `include/kinetix/ui/design_tokens.hpp` (`ThemePalette::dark_obsidian()` and `ThemePalette::light_archival()`) and verified via `include/kinetix/ui/ux_design_system.hpp` (`compute_wcag_contrast_ratio`):

- **Primary Workspace Surfaces (`color.background`, `color.surface`)**: Deep Slate-Teal (`#0D181B` to `#172A2E`) in Dark Studio mode (`15.6:1` WCAG AAA contrast against `#F4EFE6` primary text) and Warm Archival Ivory (`#F5F0E6` to `#FCF9F2`) in Light Archival mode (`14.9:1` WCAG AAA contrast against `#16272B` primary text).
- **Brand & Interactive Accents (`color.primary`, `color.secondary`)**: Kinetic Mint (`#91D1B5` dark / `#1C7B5F` light) designates primary actions, active navigation pills, and unlocked productive gates. Terracotta Coral (`#EE9A7C` dark / `#B6573D` light) highlights the `L2` brand mark, 90% FSRS retention floors, and prosodic F0 pitch contours.
- **Multi-Modal Status Semantics (`color.success`, `color.warning`, `color.error`)**: Every status badge pairs its semantic color with an explicit text label and non-color prefix symbol (`[✓ PROD UNLOCKED]`, `[⏳ REC GATE <3d]`, `[! LEECH ACTIVE]`) so status is never communicated by color alone.

## Typography

All text is shaped natively through **PangoCairo (`libpangocairo-1.0`) + HarfBuzz (`libharfbuzz`)** in `src/ui/vector_canvas.cpp`:

- **Display & Section Headings (`type.display.fontFamily: "Inter Display"`)**: Semi-Bold `16px–30px` for hero action prompts, lemma headers, and scientific chart titles.
- **UI Body & IPA Phonetics (`type.body.fontFamily: "Inter"`)**: Regular/Semi-Bold `11px–16px` with full Unicode IPA support (`[ɛntˈʃʊldɪɡʊŋ]`).
- **CJK Multi-Script Fallback (`type.cjk.fontFamily: "Noto Sans CJK JP"`)**: Seamless glyph shaping for Japanese Kanji, Hiragana, and Katakana (`遅れてすみません`, `〜なければなりません`).
- **Tabular Telemetry & Timers (`type.monospace.fontFamily: "Noto Sans Mono"`)**: Used strictly for aligned FSRS stability intervals (`12.6d`), millisecond latency gates (`1,380 ms`), Zipf ranks (`#42`), and UUID revlog columns.

## Layout

Layout composition is governed by declarative functional C++20 combinators (`photon::htile`, `photon::vtile`, `photon::box_rect::inset`, and `photon::compute_desktop_dockspace` in `include/kinetix/ui/photon_elements.hpp`):

- **Consumer Focus Studio (Default Desktop `1440×880`)**: Top command bar (`56px`), left 5-stage navigation & curriculum rail (`242px`), right Concept & 4-Modality Mastery Inspector (`324px`), bottom 4-modality editorial track bar (`30px`), and a spacious `874×794` central studio canvas with `14px` margins.
- **Pro Telemetry Dockspace (`pro_telemetry_mode_ == true`)**: Dynamically splits the central region to reveal a `156px` bottom diagnostic console containing the sortable CRDT `revlog` stream and `ModalityStatesSoA` / `PoolAllocator` profiler.
- **Responsive Mobile Ergonomics (`440×860`)**: Collapses side docks into a thumb-friendly 5-tab bottom navigation bar (`62px` height, `>= 44px` touch targets).

## Elevation & Depth

Surface hierarchy combines structural borders with an $O(1)$ sliding-window **Separable Box/Gaussian Blur & Frosted-Glass Compositor** (`VectorCanvas::apply_gaussian_blur_region` and `apply_perceptual_noise_grain` in `src/ui/vector_canvas.cpp`):

- **Ambient Radial Mesh Backdrop**: Subtle radial blooms of Kinetic Mint and Terracotta Coral illuminate the studio canvas behind cards.
- **Frosted-Glass Cards (`draw_photon_card`)**: Elevated cards blur the underlying canvas region (`radius = 5px`), apply a `0.8%` monochromatic perceptual noise grain to prevent 8-bit gradient banding, render a 1px top specular highlight (`border_specular`), and remain bounded by explicit 1px structural borders (`border.subtle`) so boundaries never rely on shadows alone.

## Shapes

Corner geometry follows a restrained three-tier scale defined in `SpacingTokens`:

- **Interactive Controls & Badges (`radius.control: 8px`)**: Buttons, language selector pills, search boxes, and inline interlinear token highlights.
- **Inspector & KPI Panels (`radius.panel: 12px`)**: Secondary cards, FSRS forgetting curve panels, and 4-modality radar containers.
- **Primary Hero & Stage Surfaces (`radius.dialog: 16px`)**: Central Focus Studio hero cards and primary stage workbenches.

## Components

All interactive components in `src/ui/main_window.cpp` and `src/ui/vector_canvas.cpp` share centralized token definitions and explicit state handling (`ux::ControlState`):

- **Primary & Secondary Action Buttons**: High-contrast `text_on_primary()` ink (`#112024` on Kinetic Mint `#91D1B5` in dark mode; `#F5F0E6` on Deep Emerald `#1C7B5F` in light mode), visible `2.2px` `[Tab]` focus rings (`border_focus`), and scoped Dear ImGui widget IDs (`imgui_ctx_.RegisterWidgetId`).
- **Search Filter & Empty-State Recovery**: Live substring filtering across target forms and glosses (`set_search_filter`). When a filter matches `0` curriculum nodes, the left rail renders an explicit **Empty State** explaining why no nodes are shown and providing a 1-click `[Clear Search Filter (Esc)]` recovery button.
- **Reduced-Motion Accessibility Toggle (`[Motion: Spring / Reduced]`)**: Honors `ux::MotionPreference::ReducedMotion` (`set_reduced_motion(true)`), snapping `SpringValue` transitions immediately to their target values without oscillation.
- **Safe Destructive/Recovery Actions**: Anti-Leech counter resets (`Reset Leech Counter (Safe Undo)`) and Resilient Vacation Shifts (`+3d` timestamp shift) preserve all historical CRDT `revlog` entries and can be toggled safely without data loss.

## Do's and Don'ts

- **Do** render all brand marks (`kinetix-l2-logo-dark.svg`, `kinetix-l2-logo-light.svg`, `kinetix-l2-mark-*.svg`) and duotone icons natively through `librsvg-2.so.2` (`include/third_party/rsvg_abi.h`).
- **Do** pair every color-coded FSRS or WaniKani gate badge with an explicit text label and non-color symbol (`✓`, `⏳`, `!`).
- **Do** support keyboard-only navigation (`[F1–F5]` stage switching, `[Tab]` focus ring cycling, `[Enter]` activation, `[Esc]` search filter clearing) and `Reduced Motion` mode.
- **Don't** introduce HTML, CSS, JavaScript, WebView, Electron, or generic purple-blue AI gradients anywhere in the application.
- **Don't** display raw engineering profiler tables in the default Consumer Focus Studio view unless the user enables `Pro Telemetry: ON`.
