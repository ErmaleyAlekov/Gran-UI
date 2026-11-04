# Changelog

[Русский](CHANGELOG.md) · **English**

All notable changes to Gran UI. Format — [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
versions — by [semantic versioning](https://semver.org/), release branch rules and guarantees —
in [docs/versioning.en.md](docs/versioning.en.md), plans — in [docs/roadmap.md](docs/roadmap.en.md).

Version sections: "Added", "Changed", "Incompatible Changes", "Deprecated", "Removed", "Fixed", "Security".

## [Unreleased]

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
