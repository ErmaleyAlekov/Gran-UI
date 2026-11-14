# Gran UI Roadmap

[Русский](roadmap.md) · **English**

← [Back to the overview](../README.en.md)

> **This is a living document.** Here is only what has not been done yet and ideas for the future. The order of tasks, their composition
> and deadlines **will change** as the engine develops: based on user feedback, what is discovered during implementation, and as new tasks appear. Dates are guidelines, not promises. Done items are moved to the [CHANGELOG](../CHANGELOG.en.md), versioning rules are in [docs/versioning.en.md](versioning.en.md).
>
> Updated: 2026-11-10 (the 2.1 window frame is done in main and goes into 0.2.0 together with CI and the release build).

## Table of Contents

1. [Direction](#direction)
2. [First Priority: macOS without an Own Mac](#first-priority-macos-without-an-own-mac)
3. [Releases](#releases)
4. [Production Readiness](#production-readiness)
5. [Architectural Risks](#architectural-risks)
6. [Stage 2. Controls for Large Programs](#stage-2-controls-for-large-programs)
7. [Stage 3. Appearance, Printing, Networking](#stage-3-appearance-printing-networking)
8. [Stage 4. Russian OS](#stage-4-russian-os)
9. [Stage 5. Interface in Games](#stage-5-interface-in-games)
10. [Cross-cutting Tasks](#cross-cutting-tasks)
11. [Follow-ups to Completed Work](#follow-ups-to-completed-work)
12. [Postponed](#postponed)
13. [Future Ideas](#future-ideas)

## Direction

The main goal is **an own foundation for desktop programs of any complexity for any order**: Windows, Linux (including
the Russian OSes — Astra Linux, Alt, RED OS) and macOS; programs are written in C++ or through the Python, C#, Java wrappers. The second line is the interface inside games on the same core. There is no intention to repeat everything that exists in Qt: the engine grows from the tasks of real programs.

Rules for each item in the plan:

- works on Windows **and** Linux; new items are checked with unit tests and screenshots;
- new elements and attributes are immediately added to the `gran-ui.xsd` schema, accessibility (UI Automation) and C-interface with Python, C#, Java wrappers;
- RU and EN documentation is updated in the same change;
- the overarching idea is to use Beresta not only at load time: live DOM, XPath (bindings, queries, tests),
  XSD (markup validation, input field types, design rules through `xs:assert`), SAX (large data),
  XInclude (file assembly).

Stage 1 "Program Foundation" is complete (events, commands, undo, live document, binding expressions, components,
data models, windows, and program) — it is included in version 0.1.0.

## First Priority: macOS without an Own Mac

Decision of 2026-11-12: the macOS preparation is the **first priority**, ahead of the stage 2 items. There is no own Mac yet
(the purchase comes later), but most of the work can be done and checked right now: the repository is public, and for public
repositories the standard GitHub Actions runners, including macOS on Apple Silicon, are free. The former remark about expensive
macOS minutes (a multiplier for a private repository) no longer applies.

| Step | What | How it is checked without a Mac |
|---|---|---|
| **M1** ✓ | Done 2026-11-13 ([architecture](architecture.en.md#the-platform-contract-platform)): the `platform::` contract (window, event loop, keys with `Mod` = Ctrl/Cmd, IME, clipboard, cursors, fractional scale, precise scrolling) in the architecture docs; a search for Win32/X11 "leaks" in the shared code | code review, Windows/Linux builds |
| **M2** ✓ | Done 2026-11-13 ([tests without a window](development.en.md#ui-tests-without-a-window-appdriver)): a windowless platform for tests (events from the test, the frame from memory), R.1 and C.2 | `make test` on Windows and Linux |
| **M3** ✓ | Done 2026-11-14 ([building on macOS](building.en.md#macos)): the core without an OS window is built with clang (the Makefile — `gmake`, CMake — including a universal x86_64 + arm64 build), all the unit tests and the crash report pass on a macOS 15 runner (x86_64 — through Rosetta); the `macos.yml` workflow, as well as `linux.yml` and `windows.yml` — on every push | a macOS runner in CI |
| **M4** | Shaders for Metal: the same HLSL → dxc → SPIR-V → MSL (SPIRV-Cross at build time, like dxc a tool, not an engine dependency), generated arrays, a `xcrun metal` check | compiling the shaders on the runner |
| **M5** | An off-screen Metal backend (`src/rhi/metal/`): device, buffers, textures, a render pass, frame readback; snapshots against references with tolerance | always the build; the snapshots only if the runner has a Metal device (first of all a `MTLCreateSystemDefaultDevice` probe in CI: Apple Silicon runners may have no GPU) |
| **M6** | The Cocoa platform (`src/platform/cocoa/*.mm`): a window with CAMetalLayer, Retina, input, clipboard, cursors | the build and an in-process test (`[NSApp postEvent:]`, the frame by own readback) on the runner |

What cannot be checked without a Mac waits for buying a Mac or renting a cloud Mac at checkpoints: the trackpad and inertia,
IME (CJK, dead keys), VoiceOver, the font look on Retina, signing and notarizing the `.app`. MoltenVK is not used: it is a
third-party library (the "no dependencies" rule), and on a runner without a GPU it would not give a live frame either.

## Releases

How branches are structured and what is guaranteed on a release branch — in [docs/versioning.en.md](versioning.en.md).
A minor version is released when its main plan item is ready; patch versions as fixes land
(a serious bug is fixed within a week).

| Version | Target | Main Content |
|---|---|---|
| **0.1.0** | released 2026-11-09 | First release with a tag: stage 1 completely, semantic versions, `gran_export.h`, sanitizer builds, the first manual check tasks (P.16), `release/0.1` branch |
| **0.2.0** | December 2026 | 2.1 Window frame (done, checked by hand); macOS without a Mac — M1–M3 (first priority); CI on each commit (P.1) with sanitizers and GPU debug layers (P.16), release build by tag (P.2) |
| **0.3.0** | January 2027 | macOS without a Mac — M4–M6; 2.2 TreeView and DataGrid; ABI check on release branches (P.3, P.4); video memory accounting and a device loss test (R.2) |
| **0.4.0** | January–February 2027 | 2.3 Input fields and forms; fallback content for `<WebView>` without a browser (R.5) |
| **0.5.0** | February 2027 | 2.4 Feedback (notifications, indicators, wizard) |
| **0.6.0** | March 2027 | 2.5 Text for editing (formatted text editor, code editor, IME) |
| **0.7.0** | March–April 2027 | 2.6 Charts for dashboards |
| **0.8.0** | April 2027 | Stabilization: integration package (P.5), fuzzing and sanitizers (P.7), analyzers (P.8), wrapper versions (P.6) |
| **1.0.0-rc.N** | May 2027 | Candidates: API and schema freeze, only fixes, testing on AMD/NVIDIA/Intel (P.12) |
| **1.0.0** | Second quarter 2027 | First stable version: from here on, incompatible changes only in 2.0 |
| **1.x** | mid-2027 | One minor version per item of stages 3–5, roughly every 6–8 weeks |

The condition for 1.0 release is not a date, but readiness: stage 2 is complete, items P.1–P.8 and P.16 are done, the architectural risk work marked "before 1.0" (R.1–R.3) is done,
manual check tasks marked "blocks the release" are closed, C-ABI and markup schema
are reviewed for "what is shameful to freeze" (if necessary, ABI is increased to 2 **before** 1.0), and at least one
minor version has lived without incompatible changes.

## Production Readiness

Recommendations without which it is hard to use the engine in serious programs. P.N numbers are used in the release table.

**P.1. Continuous Integration (CI).** Currently, checks are run manually. Needed: GitHub Actions and Gitverse workflows for every commit to `main`, `release/*`, and merge requests — Windows (MinGW + make, MSVC + CMake), Linux (GCC 8 in the Astra Linux SE 1.7 image — the oldest target glibc; fresh GCC and Clang), `make test`, `make test-render` on WARP and lavapipe, Python/C#/Java wrapper tests; compiler warnings in CI — error. Done when a branch cannot be merged with a red check.

**P.2. Release Build by Tag.** Tag `vX.Y.Z` triggers the build of archives: Windows x64 (MinGW and MSVC), Linux x86_64 (built in Astra 1.7, works on newer distributions), headers, `libgran.a`, `gran_c`, tools (`gran_rcc`, `gran_snapshot`), SHA-256 checksums, and description from CHANGELOG. Later — aarch64.

**P.3. Binary Interface Verification.** For each public structure in `gran.h` — `static_assert` on `sizeof` and `offsetof` fields (already done for GPU structures). A custom script compares the list of exported symbols in `gran_c` and signatures in `gran.h` with tag `vX.Y.0`; any difference on the `release/X.Y` branch — CI failure.

**P.4. Markup Compatibility Check.** The markup corpus (examples in `samples/`, documentation fragments, tests) version X.Y.0 must pass the release branch schema. Schema comparison: only fixes are allowed on the branch, only extensions (new elements and optional attributes) in the minor version.

**P.5. Integration Package.** `cmake --install` distributes headers, libraries, and `GranUIConfig.cmake` with `GranUIConfigVersion.cmake` (compatibility `SameMinorVersion` up to 1.0, `SameMajorVersion` after), so the program writes `find_package(GranUI 0.3 CONFIG REQUIRED)`; file `gran-ui.pc` for pkg-config; correct soname for the Linux library `libgran_c.so.<ABI>`; VERSIONINFO resource with the number from `gran_version.h`, icon, and manifest for `gran_c.dll` and example programs. The build checks the minimum version of Beresta.

**P.6. Version in Wrappers.** Python `granui.__version__`, C# `AssemblyVersion`, Java `Implementation-Version` take the number from `gran_version.h`; on loading, the wrapper compares `gran_abi_version()` (already exists) and logs a warning if the minor version of the DLL differs from its own.

**P.7. Sanitizers and Fuzzing** (cross-cutting task C.4). Before 1.0, mandatory: nightly test runs with ASan and UBSan, TSan on a task pool, `post` and data source streams; fuzzers for markup, styles, PNG, JPEG, SVG, TrueType fonts, and JSON; found inputs — into the test suite.

**P.8. Static Analysis.** *Started 2026-11-12:* `.clang-tidy` in the repository, the first run triaged (bugs fixed,
clang-analyzer findings — 0; the rest is noise and deliberate places). Left: CI, MSVC `/analyze`, cppcheck.
clang-tidy (groups bugprone, cert, performance, concurrency), MSVC `/analyze`, cppcheck — in CI, no accumulated "debt": new warning — failure.

**P.9. Supply Chain Security.** Signed tags (`git tag -s`), SHA-256 and archive release signatures, SBOM in CycloneDX format (engine, Beresta, system libraries), Authenticode signature for exe and dll on Windows, table of supported versions and response times for vulnerability reports in [SECURITY.md](../SECURITY.en.md).

**P.10. Upgrade Documentation.** In each minor version — a "How to Upgrade from 0.X to 0.Y" section with before/after examples; API reference, generated by its own generator from bilingual header comments; website documentation with version switcher.

**P.11. Performance as Regression** (cross-cutting task C.5). Measurements of startup, memory, frame time, and program size on standard markups in CI; more than 10% degradation without explanation — failure.

**P.12. Hardware and OS Testing.** Before each minor version — AMD, NVIDIA, and integrated Intel GPUs; OS — Windows 10 22H2, Windows 11, Astra Linux SE 1.7 and 1.8, Alt p10 and p11, RED OS 7.3 and 8, Ubuntu LTS. Result — table in the release description.

**P.13. Thread Contracts and Lifetime.** Each public class and function in the header specifies: which thread to call from, who owns the object, what makes references invalid. Thread checks already in the C interface are propagated to `gran::Application` in debug builds.

**P.14. Observability.** Log level from environment variable and settings; at the start of the log — engine version, ABI, Beresta, OS, GPU, and selected backend (part already written), so a crash report immediately indicates where it occurred.

**P.15. Markup Schema Version.** Each minor version schema is published at a constant address, and markup can specify the required version (attribute or namespace) — then the editor checks it with the schema of the engine version it was written for, and the engine warns about markup from a newer version.

**P.16. Quality loop for code written with AI.** Most of the engine's code is written by language models under human control, and errors in shaders, GPU buffers, and manual memory management do not immediately cause crashes—they corrupt frames or memory after a few minutes. Therefore, each major enhancement has three levels of verification:
1. **Automated:** unit tests and frame snapshots; builds with sanitizers—`make DEBUG=1 SANITIZE=address,undefined test` on Linux (GCC/Clang), CMake `-DGRAN_SANITIZE=address` with MSVC on Windows (implemented in 0.1.0); TSan for thread code; Valgrind memcheck for own parsers; debug layers of D3D12 (with GPU-Based Validation) and Vulkan (with synchronization validation) when running snapshots—any message from the layer is considered an error.
2. **Shader and GPU structure rules:** dxc with `-Wall -WX`, SPIR-V `spirv-val` check, `static_assert` on the size and offset of each structure, common to C++ and HLSL, index restrictions in shaders in debug builds.
3. **Human in the loop:** after each major enhancement, detailed manual verification tasks are prepared ([tests/manual](../tests/manual/README.md)): live window, how the frame looks, frame capture in RenderDoc or PIX, debugging check, long sessions, real GPUs, and Linux desktops. In the analysis of each task, it is noted what the person found and what the automation missed—this becomes a new automated check.

In CI (P.1), this set is run on every commit: sanitizers—on Linux, debug layers—on WARP and lavapipe.

## Architectural Risks

Areas where the engine may "hit a wall" as it grows, and what we do in advance to avoid rewriting later.
The section is compiled according to the recommendations of the project's lead developer (2026-11-04).

### R.1 Platform portability (preparing for macOS in advance)

**Currently:** Windows 10/11 (Win32) and Linux (X11/XWayland). **Risk:** porting to macOS will require a new window, input, and keyboard abstraction; if the `platform::` interface inadvertently mimics Win32 or X11, the port will become a redesign.
**What we do:**
- describe the `platform::` contract (window, event loop, keys, text input, clipboard, cursors, DPI) in the architecture documentation and check it for "leaks" specific to the OS: physical and logical key codes, `Mod` modifier (Ctrl on Windows/Linux, Cmd on macOS), IME protocol (composition string, candidate window position), precise trackpad scrolling, fractional scaling;
- create a third implementation—a "headless" platform for tests (events are provided from the test, the frame is read from memory): it speeds up interface tests (C.2) and immediately shows where the abstraction holds onto Win32/X11;
- **when:** the contract and the windowless platform are the first priority (steps M1–M2 of the "First Priority" section); the port — steps M3–M6 there.

### R.2 Video memory and long sessions

**Currently:** D3D12, D3D11, Vulkan, software WARP and lavapipe. **Risk:** transitioning between backends (backup path D3D12 → D3D11, device loss, GPU change) requires consistently recreating pipelines, texture caches, and atlases; during sessions in hours and days, video memory can grow and fragment (glyph atlas, texture table, image buffers).
**What we do:**
- video memory accounting: budget and usage through `IDXGIAdapter3::QueryVideoMemoryInfo` and `VK_EXT_memory_budget`, engine counters (atlas, textures, upload buffers)—in logs and in the inspector (C.1);
- large block allocation with suballocation (instead of separate allocation for each resource), reuse of freed areas, compaction of the glyph atlas when fragmentation exceeds a threshold;
- recreation check: a test that forcibly "loses" the device (D3D12 `RemoveDevice`, backend switch on the fly) and compares frames before and after—the scene is stored on the CPU, so the frames must match;
- "long run": a window with animations, scrolling, and image changes for 8–24 hours with process memory and video memory recording every minute; growth beyond the defined threshold—error; manual task on real GPUs;
- **when:** memory accounting and device loss test—0.3–0.5; suballocation and compaction—before 1.0.

### R.3 Text: CJK, OpenType capabilities, right-to-left writing

**Currently:** own TrueType parsing, GPU atlas, kerning, font replacement for CJK. **Risk:** CJK texts (thousands of different glyphs), OpenType capabilities (ligatures, variable fonts), and right-to-left writing will require extending the text shaping engine; if the text model is designed for "one character—one glyph from left to right," this will require a rewrite.
**What we do:**
- already keep the text model ready for growth: clusters (multiple characters ↔ multiple glyphs), separate logical and visual order, cursor and selection—by clusters; tests for this without complex writing systems (Latin ligatures, combining diacritics);
- atlas from multiple pages with eviction of rarely used glyphs— for CJK (together with R.2);
- GSUB (ligatures), variable fonts, colored emojis—item 3.1;
- Unicode bidirectional text algorithm (UAX #9) and mirrored layout—remain in "Postponed", but text shaping interfaces are designed to add them without changing the public API;
- **when:** cluster model and multi-page atlas—before 1.0; the rest—3.1 and on request.

### R.4 Code generated by language models

**Currently:** human review, unit tests, frame snapshots. **Risk:** shaders and GPU buffer work written by the model may look correct and pass tests but read beyond buffer boundaries, race on data between CPU and GPU, or leak. **What we do:** quality loop P.16—sanitizers, Valgrind, GPU debug layers, RenderDoc and PIX, shader rules, and manual verification tasks; code rules for models (`gpu_rules`) are updated with each found error. **When:** builds with sanitizers and manual tasks—since 0.1.0; everything in CI—since 0.2.0.

### R.5 Embedded browser on Linux

**Currently:** `<WebView>` works only on Windows (WebView2); on Linux, it is an empty block. **Risk:** programs that need web content on Linux receive a hole in the interface. **Options:**
1. **WebKitGTK via `dlopen`** (as already done with Vulkan): no dependency at build time, available in repositories of Astra, Alt, and RED OS, separate web process. Complexity—embedding a GTK window in an X11 engine window and synchronizing event loops.
2. **Embedded Chromium (CEF):** consistent behavior everywhere, but +100–200 MB to the package and its own security update.
3. **Leave WebView only for Windows:** honestly document it, and on other OSes show fallback content.
**Recommendation:** immediately—option 3 as a mandatory foundation: child content of `<WebView>` is displayed when the browser is not available, and the `$webViewAvailable` variable for markup; then—option 1 for Linux, if a program needs it. CEF—only by direct request. **When:** fallback content—0.4; WebKitGTK—stage 3 or 4.

## Stage 2. Controls for Large Programs

After this stage: accounting systems, admin panels, catalogs, forms.

### 2.1 Window Frame — done (goes into 0.2.0)

The menu bar with access letters, toolbars with the "more" menu, the status bar, the pane splitter, document tabs,
dockable panels with tearing off into a window, file dragging on Linux — in detail in the [CHANGELOG](../CHANGELOG.en.md),
the "Unreleased" section, and in the [guide](guide.en.md#window-frame-menus-toolbars-status-bar).

### 2.2 TreeView and DataGrid

- Column sorting, filter, cell editing (any control element as a cell editor), column width and order change, pinned columns and rows.
- Virtualization of rows **and** columns, rows of different heights (currently, the virtual list takes the average size).
- Models already exist (`ListModel`, `TreeModel`, `TreeList`, `SortFilterProxy`, `SelectionModel`) — this item adds their views.
- Accessibility: UI Automation Grid, Table, Tree, ExpandCollapse, Selection patterns.

### 2.3 Input Fields and Forms

- Elements: NumberBox/SpinBox, ComboBox with editing and autocomplete, search field, password, input mask, DatePicker and calendar, TimePicker, ColorPicker, RangeSlider, tag field.
- **Forms in the spirit of XForms:** field type and constraints taken from XSD (length, pattern, range, enumeration); `required`, `readonly`, `relevant` — binding expressions; validation messages shown next to the field and taken from the message catalog.

### 2.4 Feedback

- Notifications (toast), InfoBar, indeterminate progress and BusyIndicator, Expander, GroupBox, wizard (step-by-step dialog), breadcrumbs.
- Improvements to popup layers: drawing order by opening stack, animation of Switch slider, UIA ExpandCollapse, Selection, and Scroll patterns.

### 2.5 Editable Text

- **Formatted Text Editor:** `Span` sections, lists, tables, images, undo, input and output of Markdown and HTML.
- Text selection through multiple `Text` elements (e.g., the entire response in a chat).
- **Code Editor:** piece table, rendering only visible lines, line numbers, syntax highlighting by program tokens, search and replace, block folding.
- IME with a candidate window at the caret (Chinese, Japanese, Korean input).

### 2.6 Charts for Dashboards

- Line, bar, pie, area charts; real-time updates; axes, legend, tooltips.
- Scales and indicators for industrial interfaces (HMI).

## Stage 3. Appearance, Printing, Networking

After this stage: programs that can be confidently handed over to government and large organizations.

- **3.1 Fonts:** OpenType CFF (`.otf`), variable fonts, WOFF and WOFF2; colored emojis (COLR/CPAL, CBDT);
  system font selection based on Unicode ranges; Latin and Cyrillic ligatures (basic GSUB). Complex
  writing systems are postponed.
- **3.2 Vector Graphics:** paths, strokes, fills, clipping by path (`<Path>`, API `onPaint`); SVG in full —
  strokes, transform, clip, mask, use/defs, text — including embedded fragments in the markup.
- **3.3 Printing and PDF:** own PDF writer (vector, font subsets), printing through OS, preview.
  Business applications constantly print documents.
- **3.4 Node Effects:** shadow and blur for any node, masks, blending modes; GIF, WebP, BMP, ICO images;
  EXIF orientation for JPEG.
- **3.5 Translations:** XLIFF 2.0 catalogs, `text="@msg:id"`, language switching on the fly, plural forms for Russian
  and English, number and date formats.
- **3.6 Networking:** HTTP/1.1 and WebSocket; Windows — WinHTTP (TLS from the operating system); Linux — system OpenSSL
  via `dlopen` (as already done with Vulkan), the same way — GOST-TLS through cryptoproviders; without TLS — own
  client for localhost.
- **3.7 Integration with OS:** system theme (light/dark, accent color, high contrast, text scaling),
  notification area icon and system notifications, single instance of the program with argument passing,
  file associations, multiple monitors.

## Stage 4. Russian OS

After this stage: delivery of programs to the government sector.

- **4.1 .deb packages (Astra) and .rpm packages (RED OS, Alt):** building in the oldest target environment (Docker
  stands are already used for checks).
- **4.2 Astra Linux SE:** signing executable files `bsign` for a closed software environment, operation under mandatory
  integrity control and mandatory access control, checks on a stand with enabled modes.
- **4.3 Software rendering without a GPU** for virtual machines, VDI, terminal servers, and Elbrus without
  drivers; find out if lavapipe is available out of the box in Astra, Alt, and RED OS — currently the CPU path goes only through it and WARP.
- **4.4 Other architectures:** ARM64 (Baikal) and e2k (Elbrus, LCC compiler); live X11 window check on the Fly desktop (Astra) —
  file dialogs, own frame, clipboard.

## Stage 5. Interface in Games

A separate direction on the same core.

- **5.1 Rendering in the game frame:** command list and render target of the program (D3D11, D3D12, Vulkan), without its own window
  and swap chain; input and time come from the game; game textures can be displayed in the interface.
- **5.2 Partial rendering and layer cache:** frame is built only on changes; CPU frame budget less than 1 ms; measurements.
- **5.3 Gamepad and spatial navigation** (arrows between elements), animations from code, spring animations.
- **5.4 Plugins for Unity** (C# over C-interface) **and Unreal** (C++), example game.

## Cross-cutting Tasks

They run in parallel with the stages, not at the end.

- **C.1 Live interface inspector:** node tree, computed styles, layout frames, reasons for re-rendering,
  frame time, XPath queries to the live tree, attribute editing on the fly.
- **C.2 Interface tests for application developers:** node search via XPath, event delivery, condition waiting,
  frame snapshots with tolerance.
- **C.3 `gran_lint` and `gran_compile`:** XSD markup validation, key/keyref and `xs:assert` rules (contrast,
  touch area size, accessibility labels); pre-compilation of markup for quick start.
- **C.4 Fuzzing and sanitizers** — see P.7.
- **C.5 Comparative measurements** with Qt Widgets, Qt Quick, Electron, and WPF on the same programs: memory, start,
  frame, size — see also P.11.
- **C.6 Release:** version number in exe resources, icon, and manifest, checks on NVIDIA and Intel — see P.5 and P.12.
  (Semantic versions, `gran_export.h` and CHANGELOG — done in 0.1.0.)

## Follow-ups to Completed Work

Small loose ends of finished parts. They are picked up along with stage items that touch the same area.

**Commands and Keys.** System (outside the program) hotkeys; the `+`, `-`, `/` keys in shortcuts;
commands with a checked state; `enabled="{…}"` on `<Command>`.

**Undo and Redo.** Macro steps (`beginMacro`/`endMacro`) — a single undo step for a group of actions; undo in UI Automation;
window title API (unsaved document asterisk).

**Live Document.** Editing attributes of `<Window>` and `<Resources>` (styles, theme) through a live DOM without a full
reload; insertion and reordering of elements through the DOM; when a child is inserted, match nodes by `id`
instead of rebuilding the whole parent.

**Binding Expressions.** Expressions in `Style` and states; new `<Variable>` on partial reload; searching
for a record copy without linear traversal (currently O(n) per binding); node properties (`$id.property`) in view filters.

**Components.** Parameters in `style` and enumerations; components declared directly in the window; `xpointer`
and `xi:include` inside libraries.

**Models.** UI Automation for lists with selection; lazy trees from files (currently the entire tree is read).

**Windows and Program.** Own XSD for program settings values; `<WebView>` with content from `res:/`;
stack overflow interception in task pool threads (Linux); centering windows considering multiple monitors in X11.

**Text.** Lines of different heights within a single block (section with a larger font); parsing Markdown only the changed
tail during streaming output.

**Lists and Layout.** Virtualization of `Grid`; traversing children of a very long non-virtualized stack without O(n) per pass.

**Window Frame.** Scrolling the tab strip by arrows (now the overflow is a menu); the dock edge length from the keyboard
at the edge handle; keeping torn-off panels between runs; access letters on buttons and field labels (now — menus only);
UI Automation events about expanding and collapsing menus; panels of one dock edge side by side (now — as tabs only).

**Popup Layers.** Submenu close delay when moving the mouse
diagonally; ComboBox width by the longest item; ComboBox label when editing the list model.

**Widgets and Animations.** `animationPlayState` and pause; restarting animation from code; mixing
transformations of different forms through matrices (currently — jump); animations in `gran_snapshot` at a specified time.

**Accessibility.** ScrollPattern on Scroll, TextPattern (caret and selection for screen readers), LiveRegion
(announcements about text changes not in focus, for example, status), value change events on nodes not in focus,
LabeledBy as a UIA property.

**Images.** Fast inverse DCT for JPEG; interlaced PNG (Adam7).

**Development.** Reloading individual assets (images, fonts) without reloading the markup; applying
OS window parameters on reload.

**Linux.** Large clipboard transfers (INCR); browser for `<WebView>`.

## Postponed

We'll return when real demand arises:

- right-to-left text and complex scripts (Arabic, Hebrew, Indic);
- macOS (detailed plan — in [ideas](#macos)), Android, iOS, WebAssembly;
- 3D, video, Lottie, MathML; constraint solver, subgrid;
- AT-SPI and own Wayland (on Russian OSes, the main server is X11);
- server interface; LSP server for markup; SCXML state machines;
- Rust, Go, PHP, TypeScript wrappers;
- touch input, pen, and gestures; program auto-updates; spell checking and hyphenation by dictionaries.

## Future Ideas

Ideas discussed but not yet part of the plan. Their content may change significantly.

### User-Customizable Interface

The end user of the program does not edit XML. A button or key combination puts the window into edit mode, and directly
in it, buttons, panels, and other parts can be dragged and added, functions can be combined and split within the limits
allowed by the program. Example for a code editor: combine several file actions into one button or add a sidebar for another
assistant.

This is possible because the Gran UI interface is declarative data checked by a schema. It is built from ready parts:

1. **Edit Mode** — a layer above the window: selection frames, handles, insertion zones (node dragging and hit-test already exist).
2. **Palette** — only what the program allows: registered `<Command>` and panel-component templates
   (for example, an "assistant panel" with a "model" parameter). Adding what the program cannot do is not possible.
3. **Compound Button** — a list of commands executed in sequence (without scripts), with a common undo step.
4. **Changes** — live DOM editing with XSD validation and rollback, undo and redo of layout changes.
5. **Saving** — not a copy of the interface, but a patch overlay on the base markup by stable `id`: program updates do not erase settings, unknown `id`s are skipped with a warning; stored in the user settings directory.
6. **Security** — only layout, parameters, and links to registered commands and templates in the user file.
   The developer of the program sets boundaries: which areas can be edited (an attribute like `customizable`)
   and what is in the palette.

Place in the plan — together with dockable panels (2.1) or a separate item after stage 2.

### macOS

The steps done without a Mac (M1–M6) are in the ["First Priority"](#first-priority-macos-without-an-own-mac) section; here is the full plan.

- **Graphics** — own Metal backend (`src/rhi/metal/`), not MoltenVK: Metal is available on all supported Macs,
  no third-party library is needed. Shaders — the same HLSL: dxc → SPIR-V → MSL through SPIRV-Cross during build.
- **Platform** — Objective-C++ only in `src/platform/cocoa/*.mm`; public headers remain pure C++.
- **Steps:** core and tests without a window (CI on macOS) → Metal off-screen and snapshots against reference with tolerance → Cocoa window with CAMetalLayer and Retina → input (trackpad, IME, Cmd instead of Ctrl) → system fonts → file dialogs,
  own frame, program menu → accessibility NSAccessibility → universal binary, `.app`, signing and notarization.
- **Verification without own Mac:** macOS runners in CI for building, tests, and snapshots; renting Mac at control
  points for live testing of trackpad, IME, VoiceOver, and font appearance.

### Converting Markup to HTML

Separate utility `tools/convert` (not in the core): Gran UI XML → HTML + CSS — Stack and Grid to flex and grid, styles —
to CSS classes, states — to `:hover`/`:focus`. Needed for preview in the browser and web versions of programs and removes
the concern of "binding" to the engine.
