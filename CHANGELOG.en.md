# Changelog

[Русский](CHANGELOG.md) · **English**

All notable changes to Gran UI. Format — [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
versions — by [semantic versioning](https://semver.org/), release branch rules and guarantees —
in [docs/versioning.en.md](docs/versioning.en.md), plans — in [docs/roadmap.md](docs/roadmap.en.md).

Version sections: "Added", "Changed", "Incompatible Changes", "Deprecated", "Removed", "Fixed", "Security".

## [Unreleased]

### Added
- The `GRAN_ADAPTER` environment variable — an explicit choice of the video card by a part of its name (D3D12, D3D11, Vulkan);
  no such card — the `GPU_ADAPTER_NOT_FOUND` error rather than a silent switch to a software adapter.
- `gran_linux_tests --hardware [--window]` — Linux GPU tests on a real video card (previously lavapipe only).
- The Intel + NVIDIA, Windows + Linux [test stand](docs/test_stand.en.md): hardware, commands and run results.

### Fixed
- Windows: after quickly dragging the window edge the window froze for seconds (on Intel UHD 630 — up to 3 s for
  300 steps), black and white strips flashed at the edges. Every size change event accumulated in the modal loop
  recreated the swap chain for a size no longer needed; now the size is taken from the current window, and the extra
  recreations are skipped. The regression test is "live resize storm" in `gran_render_tests`.
- Windows: while the window edge was quickly dragged back and forth, white and black strips flashed at the growing edge
  (Intel — in a third of the monitor frames, NVIDIA — in every tenth). DWM shows the window of the new size before the
  GPU frame, and the grown strip showed the unpainted window surface. Now the window paints it with its solid
  background (`WindowDesc::background`, `WM_ERASEBKGND`), and the strip blends with the background. The measurement is
  `gran_render_tests --hardware --resize-artifacts`: a real mouse and monitor frames through Desktop Duplication (on the
  stand only).
- The CMake + MSVC build with `-DGRAN_SANITIZE=address` did not link (LNK2038 `annotate_string`): the sanitizer is now
  applied to Beresta as well.
- MSVC 14.44 (Visual Studio 2022): in release builds markup with a color like `#172033` was rejected by the schema — fixed
  in Beresta (a workaround for an optimizer bug when compiling the `a|b|c` choice of XSD regular expressions).
- Linux: on a failure of the card chosen by `GRAN_ADAPTER` the backend no longer replaces it with the software lavapipe.
- CMake: the `gran_crash_reports` crash probe is not built with a sanitizer (ASan intercepted the deliberate crash, the test hung).

## [0.1.0] — in preparation

The first release with a version number. Includes everything done up to 2026-11-04, including roadmap stage 1,
"Program Foundation".

### Added

- **Versions and compatibility guarantees:** `include/gran/gran_version.h` (the only place with the version number,
  `GRAN_VERSION_*`, `GRAN_VERSION_AT_LEAST`, `GRAN_ABI_VERSION`), `include/gran/gran_export.h` (`GRAN_API`,
  `GRAN_DEPRECATED`, `GRAN_EXPERIMENTAL` and short guarantees for `release/X.Y` branches); C interface functions
  `gran_version_number` and `gran_version_string`; CMake takes `project(VERSION)` from the header;
  [docs/versioning.en.md](docs/versioning.en.md) and the public [roadmap](docs/roadmap.en.md).
- **Quality checks:** sanitizer builds — `make DEBUG=1 SANITIZE=address,undefined` (Linux, GCC/Clang)
  and CMake `-DGRAN_SANITIZE=address` (MSVC); manual check tasks for a person — [tests/manual](tests/manual/README.md)
  (live window, RenderDoc frame capture, device loss, long sessions, accessibility).
- **Markup and schema:** XML markup with XSD 1.1 validation during parsing (value types, `xs:assert`,
  `xs:key`/`xs:keyref`), styles in CSS syntax, cascade with themes and states, transitions and keyframe animations.
- **Elements:** `Window`, `Stack`, `Grid`, `Canvas`, `Box`, `Scroll`, `Text`/`Span`, `Image`, `Icon`, `Button`, `Input`,
  `TextArea`, `CheckBox`, `Switch`, `RadioButton`, `Slider`, `ProgressBar`, `ComboBox`, `Tabs`, `Popup`, `MenuItem`,
  `Dialog`, `WebView` (Windows, WebView2); window with its own frame (`titleBar="custom"`).
- **Layout:** subset of flexbox, Grid (`fr`, `minmax`, cell merging), local reflow,
  virtual lists.
- **Text:** own TrueType/TTC parsing with `kern` and `GPOS` kerning, precise area rasterization, glyph atlas,
  line wrapping, justification, selection and clipboard.
- **Rendering:** Direct3D 12 and Direct3D 11 on Windows, Vulkan 1.0 on Linux (loaded via `dlopen`), software
  WARP and lavapipe devices, background blur, `gran_snapshot` — off-screen frame.
- **Stage 1 "Program Foundation":** event model with capturing and bubbling and node dragging (1.1);
  commands and hotkeys `<Command>` (1.2); undo and redo (1.3); live document and partial markup reloading
  (1.4); XPath 1.0 binding expressions (1.5); components and component libraries (1.6); models —
  lists, trees, sorting and filtering, selection, JSON and XML sources (1.7); modal windows and owner windows,
  task pool, log and crash report, resources inside exe (`gran_rcc`, `res:/`), settings in XML (1.8).
- **Platforms:** Windows 10/11 (Win32, UI Automation), Linux (X11/XWayland), tested on Astra Linux SE 1.7 and 1.8,
  Alt p10 and p11, Red OS 7.3 and 8.
- **Other languages:** shared `gran_c` library with C ABI 1, Python wrappers (`ctypes`), C# (P/Invoke), Java (FFM).
- **Build:** Makefile with auto-dependencies and CMake; MinGW-w64, MSVC, GCC, Clang.

### Fixed

- Undefined behaviour (found by UBSan): `saveState` dereferenced a null program pointer when a window was closed
  and the window state had been created without an `Application` (window layer unit tests).
- Found by manual testing (2026-11-06):
  - the selection in a tree (`TreeList`) was lost when a branch was collapsed and expanded; now the marks of the hidden
    descendants come back on expansion, and the current record moves to the parent on collapse
    (`ListSource::hasRowKeys`/`rowKey`);
  - the selection highlight slid to a neighbouring record after a tree branch collapse or a removal of a record above
    the selected one: the list binding recomputed it by the old copy positions — now it recomputes after its own changes;
  - Ctrl+Shift+arrow in a list only moved the current record — now it extends the range keeping the former marks;
  - `make DEBUG=1` after a release build did not rebuild the object files — the programs had no debug information;
    the Makefile keeps the flags line in `obj/.flags` and rebuilds everything when it changes;
  - log warnings in the Windows console were unreadable (UTF-8 in code page 866) — now through `WriteConsoleW`;
  - the GPU device loss record did not name the reason — now with the `GetDeviceRemovedReason` code
    (`DXGI_ERROR_DEVICE_HUNG (0x887A0006)` etc.).
