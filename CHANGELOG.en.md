# Changelog

[Русский](CHANGELOG.md) · **English**

All notable changes to Gran UI. Format — [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
versions — by [semantic versioning](https://semver.org/), release branch rules and guarantees —
in [docs/versioning.en.md](docs/versioning.en.md), plans — in [docs/roadmap.md](docs/roadmap.en.md).

Version sections: "Added", "Changed", "Incompatible Changes", "Deprecated", "Removed", "Fixed", "Security".

## [Unreleased]

### Added
- Linux: dragging files from file managers (the XDND protocol, version 5) into `onDrop` zones — as on Windows: the
  `DropTarget` state, the "not allowed" cursor outside the zones, local file paths in UTF-8 (`file://`, `%XX` decoded,
  network and other addresses skipped). The `text/uri-list` parser — `include/gran/utils/uri_list.hpp`. The session steps
  (the start with the source types, the drop, the data) are written to the log (INFO, category `x11`). A drop from Nautilus in
  GNOME on Wayland: the Mutter bridge sends XdndLeave right after XdndDrop — the window keeps the session until it gets the data.
- The `<MenuBar>` menu bar (plan 2.1): `MenuItem` items with menus under them; Alt (press and release) and F10 — the focus
  to the bar, the arrows — the neighbor item and its menu, Alt+letter — open a menu at once, a letter in an open menu —
  choose an item, hovering switches the open menu; clicking the bar does not take the focus. Access letters: `_` before
  a letter in the `text` of a menu item or a command (`__` is the sign itself), underlined while Alt is held or the menu is
  open from the keyboard. The `Open` style state (an item with an open menu or submenu, a `ComboBox` with an open list),
  the `MenuBar` and `MenuBarItem` theme styles. UI Automation: the MenuBar type, the AccessKey property, the ExpandCollapse
  pattern on items with submenus. The `Alt` key (`Key::ALT`, the name "Alt" in key events). Example — `samples/editor_frame.xml`.
- The `<ToolBar>` toolbar with `<ToolGroup>` groups and `<Separator>` separators (plan 2.1): flat buttons (the `ToolBarButton`
  theme style), when room is short the tail goes into the menu of the "more" button — items with the same commands, `onClick`
  and enabled state; the `overflowMode` attribute (`asNeeded`, `always`, `never`). `Separator` works in menus too.
  UI Automation: the ToolBar and Separator types, the "more" button named from the message catalog.
- The `<StatusBar>` status bar with `<StatusSection>` sections (plan 2.1): text and bindings in the sections, `sizeGrip="true"` —
  the window size grip at the bar end. The `windowRole="resize"` window role (drags the bottom-right corner; works in a window
  with the system frame too). UI Automation: the StatusBar type, a section is a "polite" live region: its text change is announced
  to screen readers (`Window::accessAnnounce`, the LiveRegionChanged event).
- The `<Splitter>` pane splitter (plan 2.1): two panes and a draggable handle; `position` in pixels or percents, `fixed`,
  the `minFirst`/`minSecond` limits, collapsing `collapse`/`collapsed` by a double click or Enter, the handle keys (the arrows,
  Shift, Home, End), the ↔/↕ cursor, `onChange` after a drag; `persist="position collapsed"` keeps the position (the live
  state is written into the markup before saving, `markup::writeLiveAttributes`). UI Automation: the handle is a Thumb with RangeValue.
- Document tabs (plan 2.1): the `Tabs` attributes `closable` (the cross, the middle button, Ctrl+W and Ctrl+F4 → `onClose`
  with the page), `reorderable` (reordering the headers by the mouse → `onReorder`), `overflowMenu` (the headers that do not
  fit go into a menu at the strip end), tabs from a model (`items` + `<ItemTemplate><Tab …/></ItemTemplate>`),
  Ctrl+Tab and Ctrl+Shift+Tab.
- Dockable panels `<DockHost>` / `<DockPanel>` (plan 2.1): panels at the edges around a center, the panels of one edge are
  tabs, dragging a title to another edge with the place highlighted, the edge length handles, the cross hides a panel;
  the actions `gran:dockShow`, `gran:dockHide`, `gran:dockToggle`, `gran:dockReset`, the `Application::showDockPanel` method;
  the layout is kept in the window settings by the panel ids without `persist`. UI Automation: a panel is a Pane.
- Tearing a dockable panel off into a tool window (plan 2.1, 7b): a title dropped into the middle of the host or outside it
  moves the panel content (the same nodes — the field state is kept) into a window above the main one; closing the window or
  `gran:dockShow` returns it to the edge. `Application::floatDockPanel`.

### Changed
- Windows: Alt+letter and a single Alt no longer go to the window system menu handling — no error beep and no
  "invisible" system menu mode where the next keys got lost; Alt+space still opens the system menu. Characters with
  Alt (`WM_SYSCHAR`, on X11 — characters with Alt held) come as a `CHARACTER` event with the Alt modifier and are not
  typed into inputs.
- `Renderer::render` takes the frame marks as one `render::FrameMarks` structure (the caret, the selection, the access
  letter underlines) instead of two pointers.
- In menu and menu bar item labels the `_` sign now means the access letter: the label `a_b` is shown as `ab` (for the
  sign — `a__b`). `ComboBox` item labels do not change.

### Fixed
- Document tabs: reordering by the mouse changed the order in the tree, but the headers stayed at their old places (the strip
  took the previous placement from the layout cache), and a header hidden in the menu was drawn over the menu bar.
- Dockable panels: a panel moved to an edge that already has panels showed its content without its title in the strip —
  it looked like overlapping the neighbouring panel or disappearing.
- Linux: the `.stack` file of the crash report sometimes stayed empty — when the stack pointer was closer than 4 KB to the stack
  top (it depended on ASLR); now the stack is written by page boundaries, up to the very top.
- Rebuilding a node through the live markup (`setMarkupAttribute`, restoring the settings) duplicated the parts the engine
  builds: the toolbar "more" button, the status bar and splitter handles.
- `Application::remove` of a `Tab` page left its header in the tab strip — now the header is removed together with the
  page, the choice moves to a remaining tab.
- Gallery, `samples/platform.xml`: "Save as…" only showed the chosen path and wrote nothing — now it writes a text file
  and reports "Saved: path" (found by the T-8 manual check).
- Gallery: an unknown key (for example, `–vulkan` with an en dash instead of two hyphens) is no longer taken as the markup
  file name — an "unknown key" message with a hint is printed.

### Security
- Numbers from the markup, styles and the settings file no longer cause undefined behaviour. Hundreds of digits (∞, and in the
  fractional part — NaN) and values outside the `float` range are now a parse error. Integer attributes and properties (`maxLength`,
  `rows`, the font weight, `steps(n)`, integer properties) are range-checked before the cast to an integer type: previously
  `maxLength="99999999999999999999"` passed the schema (`xs:positiveInteger` has no upper bound) and gave UB on the cast to
  `size_t`. The window size in the settings is `xs:int`; `inf` in settings numbers and `setVariableNumber(∞/NaN)` are rejected
  (in the C interface — `GRAN_INVALID_ARGUMENT`).
- The virtual list window bounds the row numbers in `double` before the cast to `size_t`.
- Static analysis: `.clang-tidy` (bugprone, cert, clang-analyzer with `EnumCastOutOfRange`, concurrency, performance), how to run —
  `docs/building.en.md`. The C interface argument checks are visible to the analyzer; `static_assert` on the style ↔ platform/text
  enumeration pairs and name tables; the keyword table size is deduced from the type.

## [0.1.0] — 2026-11-09

The first release with a version number. Includes everything done up to 2026-11-09: roadmap stage 1, "Program Foundation", and the fixes
found by manual checks, including those on the Intel + NVIDIA test stand (Windows and Linux).
Requires Beresta 2.2.1 or later.

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
- The `GRAN_ADAPTER` environment variable — an explicit choice of the video card by a part of its name (D3D12, D3D11, Vulkan);
  no such card — the `GPU_ADAPTER_NOT_FOUND` error rather than a silent switch to a software adapter.
- `gran_linux_tests --hardware [--window]` — Linux GPU tests on a real video card (previously lavapipe only).
- The Intel + NVIDIA, Windows + Linux [test stand](docs/test_stand.en.md): hardware, commands and run results.

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
- Windows, NVIDIA: while the edge was dragged the window sometimes turned completely black for 1–5 s — on the monitor
  only, a screen recording does not see it. The mouse gave several size changes per screen refresh, and for each the
  window recreated its buffers and sent a frame. Only every seventh frame reached the monitor, and the NVIDIA hardware
  planes (MPO) went dark from such a stream. Now while dragging the window waits for the DWM composition after every
  frame (`DwmFlush`): more than half of the frames reach the screen, and there is no black.
- Linux (X11/XWayland, Intel and NVIDIA): while the window edge was dragged with the mouse, the window flickered solid
  black, and on NVIDIA it could stay black after the mouse stopped. Three causes: the X11 window background was black,
  and on every resize the X server filled the whole window with it; a frame for which `vkAcquireNextImageKHR` returned
  `OUT_OF_DATE` was lost without a retry; every ConfigureNotify recreated the swap chain. Now the background is the
  `<Window background>` color, the old content is kept on resize (`NorthWestGravity`), the frame is retried right after
  the chain is recreated, and of the queued ConfigureNotify events only the last one is taken. Measured on the stand
  (240 fast resizes): black frames were 238 (NVIDIA) and 179 (Intel), now 0.
- Linux (X11/XWayland, GNOME): while the right window edge was dragged, a background strip flickered: Mutter showed the
  window of the new size before the GPU frame. The window now takes part in the `_NET_WM_SYNC_REQUEST` resize
  synchronization (an XSync counter, answered after a frame of the new size), and the manager waits for the frame.
  libXext is loaded at run time (`dlopen`), no new build dependencies. The answer goes when the frame is already in the
  window: Vulkan waits for it to be shown (`VK_KHR_present_wait`, a Vulkan 1.1 instance; at most 100 ms; no extension —
  as before); on Intel the worst answer to the manager is 145 ms instead of over 200. The remaining flicker at the right
  edge is the empty inside of the GNOME frame for X11 windows (`mutter-x11-frames`), the same for other X11 programs;
  windows with their own title bar (`titleBar="custom"`) do not have it.
- The CMake + MSVC build with `-DGRAN_SANITIZE=address` did not link (LNK2038 `annotate_string`): the sanitizer is now
  applied to Beresta as well.
- MSVC 14.44 (Visual Studio 2022): in release builds markup with a color like `#172033` was rejected by the schema — fixed
  in Beresta (a workaround for an optimizer bug when compiling the `a|b|c` choice of XSD regular expressions).
- Linux: on a failure of the card chosen by `GRAN_ADAPTER` the backend no longer replaces it with the software lavapipe.
- CMake: the `gran_crash_reports` crash probe is not built with a sanitizer (ASan intercepted the deliberate crash, the test hung).
