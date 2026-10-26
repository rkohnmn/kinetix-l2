# Native GUI Architecture & Zero-Web Verification (v2.0)

This document details how **Kinetix L2 v2.0** synthesizes three foundational C++ UI paradigms—**Dear ImGui** ([ocornut/imgui](https://github.com/ocornut/imgui)), **Cycfi Photon Micro-GUI / Elements** ([Cycfi Research](https://www.cycfi.com/2019/07/photon-micro-gui/)), and **Philippe Groarke's C++ UI Solutions Analysis** ([philippegroarke.com](https://philippegroarke.com/posts/2018/c++_ui_solutions/))—with **PangoCairo + HarfBuzz** shaped typography.

---

## 1. Synthesis of the Three C++ UI Paradigms

1. **Philippe Groarke's C++ UI Solutions Trade-Off Framework ([philippegroarke.com](https://philippegroarke.com/posts/2018/c++_ui_solutions/))**:
   - Groarke's survey highlights why traditional retained-mode C++ frameworks either require massive runtime dependencies (Qt/wxWidgets) or embed bloated web engines (CEF/Electron), whereas immediate-mode toolkits (`Dear ImGui`) excel at zero-duplication state synchronization and lightweight vector micro-GUIs (`Photon/Elements`) excel at resolution-independent custom controls.
   - Kinetix L2 v2.0 combines the strengths of both without web bloat: **Dear ImGui's immediate-mode dockspace, ID stack, virtualized list clipper, and sortable table contracts** paired with **Cycfi Photon/Elements' declarative layout combinators and vector rendering**.
2. **Dear ImGui Immediate-Mode Engineering (`include/kinetix/ui/imgui_layer.hpp` & [ocornut/imgui](https://github.com/ocornut/imgui))**:
   - **Single Source of Truth**: Domain state lives exclusively in `KnowledgeGraph` and `DatabaseManager`; UI code queries the domain directly every frame without duplicating state.
   - **6-Region Workspace Dockspace Shell**:
     1. **Region 1 (`CommandBar`)**: Global mode indicators, `Ctrl+K` live corpus search filter, language pills (`DE`, `ES`, `JA 日本語`), momentum badge, and theme/form-factor switches.
     2. **Region 2 (`LeftAssetBrowser`)**: Workspace view switcher (`F1–F5`) + `ImGuiListClipper`-backed Corpus Node Browser with 4-modality mini-sparkbars.
     3. **Region 3 (`CentralWorkspaceCanvas`)**: High-contrast primary editor/canvas for the active SLA stage.
     4. **Region 4 (`RightInspector`)**: Contextual object inspector for the selected `LinguisticNode` with 4 circular Photon `dial_gauge` rings, concept/grammar notes, 1-click sentence mining, and safe leech reset.
     5. **Region 5 (`BottomConsole`)**: Split pane housing the Event-Sourced SQLite3 WAL `revlog` table (`ImGui::BeginTable`) and the C++20 `SoA` / `PoolAllocator` Performance Profiler.
     6. **Region 6 (`StatusBar`)**: Live SQLite3 WAL status, renderer pipeline info, and keyboard shortcut hints.
   - **Automated Static & Runtime Stack Auditing**: `ImGuiContextTracker` verifies at runtime—and `scripts/check_imgui_cpp.py` verifies statically—that every `BeginWindow`/`EndWindow`, `BeginChild`/`EndChild`, `BeginTable`/`EndTable`, `PushID`/`PopID`, and `PushStyleColor`/`PopStyleColor` call is 100% balanced.
3. **Cycfi Photon Micro-GUI / Elements Declarative Combinators (`include/kinetix/ui/photon_elements.hpp` & [cycfi.com](https://www.cycfi.com/2019/07/photon-micro-gui/))**:
   - Implements Joel de Guzman's declarative layout combinators (`photon::vtile`, `photon::htile`, `photon::box_rect::inset`, `photon::box_rect::margin`, `photon::view_limits`, `photon::compute_desktop_dockspace`) plus high-craft vector widgets (`draw_photon_card` with ambient drop shadows, vertical linear gradients, and 1px top specular highlights; `draw_photon_dial_gauge` circular progress arcs).
4. **PangoCairo + HarfBuzz Multi-Script Typography (`include/third_party/pango_abi.h` & `src/ui/vector_canvas.cpp`)**:
   - Replaces Cairo's basic toy font API with full **PangoCairo (`libpangocairo-1.0.so.0`) + Pango (`libpango-1.0.so.0`) + HarfBuzz (`libharfbuzz.so.0`)** text layout, shaping **Inter**, **Inter Display**, **Noto Sans Mono**, IPA phonetic symbols (`[ɛntˈʃʊldɪɡʊŋ]`), and Japanese Kanji/Kana (`日本語`, `遅`, `〜なければなりません`) with sub-pixel anti-aliasing.

---

## 2. Automated Native GUI & ImGui Compliance Verification

Run `make audit` to execute all three verification scripts:
- `python3 scripts/native_gui_audit.py . --binary build/kinetix-l2` (Verifies zero HTML/CSS/JS/WebView/Electron in source tree and `ldd` shared library graph)
- `python3 scripts/check_imgui_cpp.py src/ui/main_window.cpp tests/test_main.cpp` (Verifies 100% `Begin`/`End` and `Push`/`Pop` balance)
- `python3 scripts/validate_project.py .` (Verifies manifests, `.env.example`, and zero `TODO`/`FIXME` placeholders)
