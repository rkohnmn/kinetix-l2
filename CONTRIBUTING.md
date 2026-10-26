# Contributing to Kinetix L2

We welcome contributions to the C++20 core engine, FSRS-4.5/5 scheduling algorithms, linguistic curriculum packs, and native platform adapters across Windows, macOS, Linux, iOS, and Android.

## Mandatory Architectural Policy: 100% Native GUI Only

Every user-facing screen, widget, chart, and animation in Kinetix L2 must be implemented through our native C++20 rendering stack (`SDL2` + `Cairo 2D Vector Graphics` + native OS adapters).

Pull requests that introduce any of the following will be rejected by CI (`scripts/native_gui_audit.py`):
- HTML, CSS, JavaScript, or TypeScript UI code
- Electron, Tauri webviews, CEF, `QWebEngineView`, `WKWebView`, `WebView2`, or Android `WebView`
- Localhost HTTP servers serving user interfaces

## Development Workflow

1. **Build and run the unit & integration test suite**:
   ```bash
   make all
   make test
   ```
2. **Run the headless verification & native GUI audit**:
   ```bash
   make verify
   make audit
   ```
3. **Code style**:
   Format C++20 files with `clang-format` using the repository `.clang-format` specification before submitting a pull request.
