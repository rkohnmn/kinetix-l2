# Third-Party Notices & Open-Source License Compliance

Kinetix L2 links dynamically against three industry-standard native C libraries. No proprietary SDKs, browser engines, or restrictive GPL dependencies are linked into the application binary.

## 1. Simple DirectMedia Layer (SDL2)
- **Version Verified**: 2.26.5 (`libSDL2-2.0.so.0`)
- **License**: zlib License (permissive commercial and open-source dynamic/static linking)
- **Role in Kinetix L2**: Native operating-system window management, high-DPI display scaling, keyboard/mouse/touch input handling, hardware texture presentation, and low-latency PCM audio subsystem across Windows, macOS, Linux, iOS, and Android.
- **Source & Notice**: https://www.libsdl.org/license.php

## 2. Cairo 2D Graphics Library
- **Version Verified**: 1.16.0 (`libcairo.so.2`)
- **License**: GNU Lesser General Public License (LGPL) version 2.1 or Mozilla Public License (MPL) version 1.1
- **Role in Kinetix L2**: Anti-aliased 2D vector graphics rendering for all custom native UI controls, FSRS-4.5 power-law forgetting curves, 4-modality radar diagrams, WaniKani prerequisite dependency graphs, and prosodic pitch contours. Dynamically linked (`lib/x86_64-linux-gnu/libcairo.so.2`) in full compliance with LGPLv2.1/MPLv1.1.
- **Source & Notice**: https://www.cairographics.org/

## 3. SQLite3 Embedded Relational Database Engine
- **Version Verified**: 3.40.1 (`libsqlite3.so.0`)
- **License**: Public Domain
- **Role in Kinetix L2**: Transactional local-first storage in Write-Ahead Logging (`WAL`) mode with foreign key constraints, storing the 4-modality learner knowledge graph and append-only CRDT `revlog` event log.
- **Source & Notice**: https://www.sqlite.org/copyright.html

## 4. Free Spaced Repetition Scheduler (FSRS-4.5 / FSRS-5) Mathematical Specification
- **Specification Author**: Jarrett Ye (`open-spaced-repetition`)
- **License**: MIT License
- **Role in Kinetix L2**: Clean-room C++20 implementation of the DSR (Difficulty, Stability, Retrievability) power-law memory equations in `src/domain/fsrs_engine.cpp`.
- **Source & Notice**: https://github.com/open-spaced-repetition/fsrs4anki
