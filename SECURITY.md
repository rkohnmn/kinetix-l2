# Security and Privacy Policy

## Local-First Privacy Architecture

Kinetix L2 is architected from the ground up as a **local-first, offline-capable native application**:
- **Zero Cloud Lock-In**: All learner state—including your 4-modality FSRS-4.5 memory parameters (`modality_states`), custom mined sentences, and append-only `revlog` history—lives locally in an ACID-compliant SQLite3 database (`WAL` mode) inside your operating system's standard user application data directory.
- **Zero Telemetry by Default**: No keystrokes, voice recordings, or review histories leave your device.
- **Zero Embedded Secrets**: The repository and compiled binaries contain zero API keys, credentials, or tracking SDKs.

## Threat Model & Hardening

1. **SQL Injection Prevention**: Every database write and parameterized query in `src/persistence/database_manager.cpp` uses prepared statements (`sqlite3_prepare_v2` + `sqlite3_bind_*`) and explicit schema `CHECK` constraints.
2. **Memory Safety in C++20**: Domain models use value semantics, RAII wrappers (`DatabaseManager`, `VectorCanvas`, `MainWindow`), bounds-checked standard containers, and zero raw owning pointers.
3. **No Embedded Browser Attack Surface**: By excluding Chromium/CEF, WebKit, and V8 from the GUI stack, Kinetix L2 eliminates the entire class of browser-engine remote code execution (RCE) and XSS vulnerabilities common to Electron/WebView desktop apps.

## Reporting a Vulnerability

If you discover a security or data-integrity issue, please open a confidential security advisory on GitHub or email the maintainers before public disclosure.
