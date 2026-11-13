# Gran UI — Engine Architecture

[Русский](architecture.md) · **English**

← [Back to Description](../README.en.md)

Declarative UI on XML over Beresta. The goal is HTML + CSS level picture quality with a startup in milliseconds, a few MB of memory and 0% CPU when idle. Only C++17, the standard library, and system SDKs (Windows SDK for D3D12, Vulkan SDK for Vulkan, Xlib on Linux).

## 1. Pipeline

```
 XML markup ──► Beresta: parsing + XSD 1.1 (xs:assert, xs:key/keyref) in one pass
        │             errors: line, column, path /Window/Stack[2]/Button
        ▼
 markup/   DOM → UI node tree (ui::Node), resources (Theme, Style, Font, Image)
        ▼
 style/    cascade: defaults ← theme ← Style ← attributes ← states (hover/pressed/focus/disabled)
        │   → ComputedStyle (POD) + transitions and keyframes (animation/)
        ▼
 layout/   Stack (subset of flexbox), Grid (px/auto/fr/minmax, span), Canvas (absolute),
        │   text (text/: TrueType font, kerning, line breaks) → LayoutBox (x, y, w, h)
        ▼
 render/   rendering list → GpuInstance instances (rectangle SDF, shadow, glyph, image,
        │   background blur) + clip table + glyph atlas + gradient ramps
        ▼
 rhi/      D3D12 | D3D11 | Vulkan | Null (recording for tests): one instanced call per segment,
           segment changes only during background blur pass (glassmorphism)
```

Retained mode: each stage is recalculated only by dirty flags (`STYLE_DIRTY` → `LAYOUT_DIRTY` → `PAINT_DIRTY`). No changes and no animations — no frame, the thread sleeps waiting for OS events.

## 2. Modules (include/ and src/ are mirrored)

| Module | What's Inside |
|---|---|
| `core/` | `GranException` + `ErrorCode`, message catalog `i18n` (ru, en), logger |
| `log/` | the process log file (a ring of the latest records without allocations, rotation) and the crash report: Windows — an SEH filter + a helper thread + `MiniDumpWriteDump` + `RtlVirtualUnwind` unwinding; Linux — signals on `sigaltstack`, an async-signal-safe report, `/proc/self/maps`, the stack bytes |
| `res/` | resources inside the exe: the bundle registry (`res:/path` → the program bytes), path normalization, streams from memory (`MemoryBuffer`), the "GRANRES1" block for the wrappers; the manifest (the `gran-resources.xsd` schema) and generating the `.cpp`/block/dependency file for `gran_rcc` |
| `settings/` | the program settings in XML with its own schema (`gran-settings.xsd`, the Memento pattern): the program values, the window place (`<Window layout>`), the `persist` node attributes and the `persist` variables; atomic writing, a damaged file goes into `.bad` |
| `utils/` | UTF-8, number and list parsing, geometry (`Vec2`, `Rect`, `Affine2D`), color |
| `markup/` | loading through Beresta, built-in schema `gran-ui.xsd`, building `ui::Node`, the live DOM (`live_document.hpp`) |
| `style/` | value parsing (color, length, margins, radii, shadows, gradients, transitions), cascade, theme |
| `ui/` | UI node, element types, states, traversal |
| `data/` | the common list interface `ListSource` (observer: insertion, deletion, range editing, movement, resorting) and its implementations: the `ListModel` storage (string records, table columns, the `ListFetcher` fetcher), proxy `SortFilterProxy` (the order of the source indices, point edits), flat tree `TreeList`; tree `TreeModel` with stable `NodeId` and lazy nodes; `SelectionModel` (the marks follow the records); comparison `record_order` (case-insensitive text, numbers, natural); sources: own JSON reader `JsonReader` (pulling, portions) and XML via Beresta SAX (the select path on the fly, a DOM of the record only, lazy loading by a worker thread with a hand-over); `{field}` substitution, the `Variable` variables |
| `binding/` | registry of models by name (`ListBindings`: lists, trees, views, selections — one namespace) and container binding `items="…"`/`selection="…"`: copies of `<ItemTemplate>`, point editing, reordering of copies on sorting, Selected state, selection by keys and clicks, fetching in the virtual window; expressions `{XPath}` (`ExpressionBindings`): mirror of models in XML (lists, trees as nested `Item`s, selections as copies of the marked records), row variables (`$selected`, `$depth`…), view filter expression (`ExpressionFilter`), dependencies, recalculation of changed, two-way writing (first collected, then applied), converters |
| `layout/` | Stack, Grid, Canvas, text measurement, scrolling |
| `text/` | TrueType/TTC parsing, glyph rasterization (exact coverage area), atlas, kerning, line breaks |
| `image/` | inflate (zlib), PNG and JPEG (baseline and progressive) — custom implementations; the format is by the signature (`decodeImage`) |
| `vector/` | icon vector shapes: SVG path parsing (with arcs), `.svg` files through Beresta, rasterizing by exact area (nonzero/evenodd) |
| `animation/` | property transitions and keyframe animations (`<Keyframes>` + `animation`), curves (`cubic-bezier`, ease, linear, steps), frame clock |
| `undo/` | the undo stack (as `QUndoStack`): `UndoStep` steps with merging, the position, the clean state, the limit; used by the input fields and the application stacks |
| `input/` | hit-test (considering transform), focus, event routing, text editing and selection, clipboard, the field edit history (Ctrl+Z/Ctrl+Y) |
| `a11y/` | accessibility: roles, names, control patterns, tree navigation, the client request queue (neutral); UI Automation provider in `platform/win32/uia_*`, the requests are performed by `InputRouter::perform` |
| `render/` | rendering list, instance packing, clips, segments, glyph cache |
| `rhi/` | backend interface, `null/`, `dx12/`, `vulkan/`, generated shaders `shaders_gen/` |
| `platform/` | window and events: `win32/`, `x11/` (then `cocoa/`); the embedded browser `webview2/` (optional) |
| `app/` | application: windows (an `AppWindow` per window — the document, the renderer, input; the shared part is in `Application::State`; the owner is a non-owning reference to a window standing earlier in the list: the owned windows close before the owners, the graphics go from the list end; the modality is the `blocked` flag of the owners, the input is dropped before the tree, on Win32 also `EnableWindow`), the background task pool `TaskPool` (a shared queue, the threads on the first task, the outcome and the merged progress — through `post`), event loop, file dialogs, file dragging, timers (`setTimeout`/`setInterval`), action binding (`onClick="name"` → C++ function), `<WebView>` placement |

### The embedded browser (hybrid)

The engine draws the interface itself and does not use a browser for this. `<WebView>` is the only place where web content appears; it is displayed by the system browser through the `platform::WebView` interface (Bridge pattern):

- `platform/webview2/` — Windows, WebView2. Compiled only with the SDK (`WEBVIEW2_SDK`); otherwise, `platform/web_view_fallback.cpp` returns `nullptr`, and the element remains a background.
- The `WebView2Loader.dll` loader is connected via `LoadLibraryEx` (program directory and System32) on the first display of the element; COM is a single-threaded apartment of the window thread.
- Creation is asynchronous: COM callbacks (`ComCallback`, without lambdas) hold a weak reference to the core, so closing during creation is safe; commands are queued until readiness.
- After each frame, `app::syncWebViews` iterates through visible nodes, calculates the content block in physical pixels and the visible part (clipping `Scroll`/`overflow`), creates the browser on the first display, and only passes changes. The visible part is set by the child frame window, and the page inside it is shifted by the invisible part.
- Page messages are queued and processed after window events (`deliverWebMessages`), not inside the browser callback.
- Currently, the page is a child window above the engine frame. The next step is composition mode (`ICoreWebView2CompositionController` + DirectComposition): the page will become a visual between engine layers, pop-up layers will appear above it, rounding and transparency will be added; input will then be redirected by the engine.
- macOS — WKWebView, Linux — WebKitGTK: the same `platform::WebView` interface.


### The platform contract (`platform::`)

An OS window is the `platform::Window` interface (the Bridge pattern, `include/gran/platform/window.hpp`). Implementations:
`win32/`, `x11/`, then a windowless one for tests (step M2 of the plan) and `cocoa/` (M6). The common code (`app/`, `input/`,
`ui/`, `render/` and the other modules) relies only on what is listed below; everything OS-dependent is done by the window
implementation. Adding a platform means fulfilling this contract — there must be no changes in the common code.

| Area | Contract | Win32 / X11 | Cocoa (plan) |
|---|---|---|---|
| Threads | `createWindow` and all window methods — only on the UI thread, which is the main process thread. The only thread-safe method is `wake()` | any single thread; we keep the main one | the main thread is mandatory (AppKit) |
| Event loop | `pollEvent` without waiting; `waitEvents(timeout)` sleeps until an event, the timeout or `wake()`. The window procedure only queues events; the exception is `setLiveResize` (repainting while resizing), wrapped into a `noexcept` barrier | message queue / `XNextEvent` + `poll` | `nextEventMatchingMask:untilDate:`, waking — a `CFRunLoopSource` |
| Coordinates and scale | mouse events are in logical pixels of the client area; `scale()` is device pixels per logical pixel, may be fractional (1.25; 1.5); `RESIZE` and `pixelSize()` are in device pixels; moving to a monitor with another scale gives `DPI_CHANGED` | DPI v2 / `Xft.dpi` | a logical pixel is a point, `scale` = `backingScaleFactor` |
| Keys | `Key` is by the physical position (Ctrl+C works in the Russian layout); auto-repeat — repeated `KEY_DOWN` | virtual codes / keycode | `keyCode` |
| Modifiers | `MODIFIER_CONTROL` is "Mod", the primary shortcut modifier (as `Qt::ControlModifier`); `MODIFIER_ALT` is Alt/Option; `MODIFIER_META` is Win/Super, the common code does not use it | Ctrl → `CONTROL` | Cmd → `CONTROL`, the physical Control → `META` |
| Text | a character is `CHARACTER` (UTF-32) after `KEY_DOWN`; chords with Mod give no characters. Dead keys and AltGr are typed by the OS | `WM_CHAR`; AltGr = Ctrl+Alt, the field takes it | `interpretKeyEvents`; if Option typed a character — `KEY_DOWN` without `MODIFIER_ALT` |
| IME | **not in the code yet** (item 2.5): a `COMPOSITION` event (the composition string and caret), committing — `CHARACTER`, `Window::setTextInputArea(Rect)` — where to show the candidate window | IMM32/TSF, XIM/IBus | `NSTextInputClient` |
| Wheel | `wheel` is in "notches" (fractions allowed); precise sources (a trackpad) divide pixels by `WHEEL_NOTCH_PIXELS` (48). Plus on y is towards the document start, plus on x — towards the line start | `WHEEL_DELTA` = 120; X11 — buttons 4–7 | `scrollingDeltaX/Y`, with `hasPreciseScrollingDeltas` — divided by 48; the OS gives the inertia |
| Clicks | `clicks` on `MOUSE_DOWN` is the OS count (a double click by its settings); capturing the mouse until release is the implementation's job | `SetCapture` / an active grab | `clickCount` |
| Cursors | `CursorShape` (the order is as `style::Cursor`) | system cursors | `NSCursor` |
| Clipboard | UTF-8 text, synchronously (`clipboardText`, `setClipboardText`) | `CF_UNICODETEXT` / the `CLIPBOARD` owner | `NSPasteboard` |
| Window | `show()` — after the first frame (no white flash); minimize, maximize, restore, `close()` — by the usual `CLOSE` event; a custom frame (`TitleBar::CUSTOM`) and region roles (`WindowRole`) via `setFrameHitTest`; the owner and modality — the application blocks the input itself, the OS only helps | `EnableWindow` / `_NET_WM_STATE_MODAL` | `zoom:`, `miniaturize:`; a custom frame — `FullSizeContentView` + a transparent title bar |
| Window place | `WindowPlacement` is in OS coordinates, portable only within the same OS; a place outside all monitors is not applied | `rcNormalPosition` / the X11 root | the frame in screen points (the y axis from the bottom — converted by the implementation) |
| File drag and drop | `DRAG_MOVE` / `DRAG_LEAVE` / `DROP` + `takeDroppedFiles`; the OS is answered by `setDropZones` without touching the UI tree | OLE `IDropTarget` / XDND | `NSDraggingDestination` |
| Accessibility | `setAccessTree`, `accessFocusChanged`, `accessRefresh`, `accessAnnounce` over the neutral `a11y/` | UI Automation / (AT-SPI — later) | `NSAccessibility` |
| GPU surface | `nativeHandle()` (+ `nativeDisplay()` for Vulkan); `rhi/` picks the backend by the OS | `HWND` / `Window` + `Display*` | `CAMetalLayer*` |
| Frame sync | `waitsForFrame` / `framePresented` — answering the window manager on resize; not needed by default | — / `_NET_WM_SYNC_REQUEST` | not needed (the layer syncs itself) |
| File dialogs | `showFileDialog` is modal and synchronous, UTF-8 paths | `IFileOpenDialog` / zenity or kdialog | `NSOpenPanel` / `NSSavePanel` `runModal` |

The conventions the user sees are learned by the common code from `platform::hostOs()` (`host.hpp`) — the only `#if` by
the OS for them: today it is the look of the shortcut labels in menus (`Ctrl+Shift+S` or `⇧⌘S`, `ui::shortcutText`).
OS calls outside `platform/` and `rhi/` exist only in modules that need them by nature: the log and the crash report (`log/`),
the settings paths (`settings/`), file mapping (`utils/mapped_file_os.cpp`), the system font search (`text/library/`).

The Cocoa implementation translates the usual macOS chords into the canonical engine events, so text editing in the common
code is one for all OSes:

| macOS | Engine event | Meaning |
|---|---|---|
| Cmd+← / Cmd+→ | Home / End | the line edge |
| Cmd+↑ / Cmd+↓ | Mod+Home / Mod+End | the text edge |
| Option+← / Option+→ | Mod+← / Mod+→ | by a word |
| Option+Backspace / Option+Delete | Mod+Backspace / Mod+Delete | delete a word |
| Control+Tab, Control+Shift+Tab | Mod+Tab, Mod+Shift+Tab | document tabs (Cmd+Tab is taken by the system) |
| a lone Option | not sent as `Key::ALT` | macOS has no menu bar by Alt: the menu is the system one |

#### Searching for Win32/X11 "leaks" in the common code (2026-11-13, step M1)

| Finding | Outcome |
|---|---|
| Ctrl is hard-wired as the primary modifier in input (`input/`: text editing, commands, tabs, lists) | closed by the contract: `MODIFIER_CONTROL` = Mod, on macOS Cmd goes into it; `MODIFIER_META` added |
| The shortcut label was always "Ctrl+…" | `ui::shortcutText` picks the look by `hostOs()`; the macOS signs — `ui::shortcutSymbols` |
| 48 pixels per wheel notch was a number inside `input/`; the Win32 horizontal wheel scrolled the opposite way to X11 | the wheel unit is in the contract (`WHEEL_NOTCH_PIXELS`), the Win32 sign is fixed |
| The settings and log paths on macOS would land in `~/.config` and `~/.local/state` | `~/Library/Application Support/<program>` and `~/Library/Logs/<program>` (checked by the macOS build, M3) |
| The POSIX crash report: `gregs[REG_RIP]` registers (glibc), `/proc/self/maps` | will not build on macOS — step M3 (`uc_mcontext->__ss`, the `dyld` image list) |
| Makefile and CMake stop the build on macOS | step M3 |
| Modality: Win32 disables the owner, X11 cannot | already neutral: the application blocks the input (`blocked`), the OS only helps |
| The menu bar by Alt, AltGr as Ctrl+Alt | Windows/Linux conventions; Cocoa fulfils them by the table above |
| No IME on any platform | the contract extension is described above, the implementation is item 2.5 |

There is no `#if` by the OS in `app/`, `input/`, `ui/`, `render/`, `layout/`, `style/`, `markup/`, `binding/`, `data/`.

## 3. Markup and Schema

- Root — `<Window>`; resources — `<Resources>`: `<Theme>` (named colors and sizes), `<Style name=…>`
  with nested states `<Hover>`, `<Pressed>`, `<Focused>`, `<Disabled>`.
- Elements v1: `Stack`, `Grid`, `Canvas`, `Box`, `Text`, `Image`, `Button`, `Input`, `Scroll`; later added
  `CheckBox`, `Slider`, `TextArea` (multiline field: `rows`, `submitKey="ctrlEnter|enter"`, grows to `maxHeight`
  and its own scrolling), `Span` (formatted text segment inside `Text`: own font, color, `background`,
  `textDecoration`, link `onClick`), `Popup` (popup layer: hidden until `Application::showPopup`, out of the flow,
  above everything; Esc and clicking outside close it) and `MenuItem` (menu item in `Popup`). The `tooltip` attribute on any
  element — tooltip after 0.6 seconds; right-click on text and fields — built-in context menu.
- The `<Command id text action key scope within enabled>` commands in `<Resources>` (the Command pattern, as QAction):
  `Button` and `MenuItem` refer to them with the `command` attribute; the loader gives the controls the label,
  the shortcut label and enabled at once (`ui::bindCommandTree`), the application — the enabled state from code and the global
  commands of other windows. `input::matchCommand` looks up shortcuts by scopes: focus (inside `within`) → window → global.
- Attribute names — camelCase and identical for all elements (`gridRow`, `borderRadius`, `justify`, `align`).
  Differences from the concept draft: `grid-row` → `gridRow`; `alignment`/`verticalAlignment` →
  `justify` (main axis) and `align` (cross axis) — they do not depend on the orientation and are not confused.
- Built-in XSD 1.1 schema in the library checks the document **during parsing** (`ParserOptions::schema`):
  - simple types with patterns: color (`#rgb`, `#rrggbbaa`, `rgb()`, `hsl()`, `$themeName`), length, Grid track list;
  - `xs:assert` for interface constraints: `minWidth ≤ maxWidth`, Grid row/column number less than the number of tracks,
    `Button` has text or content, `Scroll` contains exactly one element, etc.;
  - `xs:unique` on `id` in the window, `xs:key`/`xs:keyref` — links `style="…"` only to declared styles.
  An error comes with the position and path before the window is created.
- **Live document.** The Beresta DOM is not thrown away after building: `UiDocument::dom` (`shared_ptr`, so that
  the public header gets by with a `beresta::Document` declaration), a node knows its element by a non-owning const
  link `ui::Node::domElement` (an element can be changed only through `internal::editableElement`). The nodes that are not
  in the markup (from code, list record copies, the Tabs strip, the ComboBox layer, the window tooltip and context menu) have
  no link. Invariant: the link is valid while the node is in a window tree — `remove` removes the element together with the node
  (`detachMarkup`), `appendMarkup` moves the fragment element into the parent DOM (`keepFragment`).
  - Editing an attribute: writing into the DOM → checking the whole DOM by the schema (`xsd::SchemaValidator`) → building a subtree
    from the element; any error rolls the attributes back. The application replaces the node **in place** (`app::rebuildInPlace`):
    the node stays the same object (the parent, listeners, hover, focus), its content and descendants are new;
    the children added at run time (`markup::isRuntimeChild`) are carried over as they are.
  - The user state (`ui/node_state.hpp`): the field text with the caret and history, check marks, values,
    the ComboBox/Tabs choice, scrolling and the focus are captured by `#id` or "ancestor key/index" keys and given back
    to the nodes of the same kind — when a node is rebuilt and on any reload.
  - Partial reload (`markup::planPatch` → `app::patchDocument`): the new document is built as a whole (the check
    and the rollback as before); if `<Window>` and `<Resources>` are equal, its DOM is compared with the live one: an equal subtree
    (`isEqualNode`) stays, for a node with the same "shell" (the name, attributes, own text, states, template) and the same
    children the children are compared, otherwise the node is rebuilt in place from the new element with the live document styles.
    All the branches are built first, then applied. A different prologue is a full replacement carrying the state over.
  - Saving: `serializeMarkup` is the Beresta serializer with indentation (the parser does not keep whitespace indentation,
    mixed content is written inline, comments are kept).
- **Binding expressions** (`binding/expression_*`, the Observer pattern + pull-based recomputation). The node builder
  only recognizes `{…}` and checks the syntax by compiling XPath (error — with position); the node stores the original text
  (`ui::AttributeBinding`), while the compilation and evaluation are in the `binding/` module: `ui/` has no dependency on XPath.
  - Data for XPath — `DataMirror`: Beresta document `<data><model><Item field="…"/>…</model></data>`; mirror entry
    (`MirrorEntry`) — model observer, editing changes one `Item` element and the model version number.
  - The dependencies are collected by an own expression scanner: `$name` (variable), `$node.property` (`self`, `parent`, id in the binding window),
    `$index`, step names of the path (possible models), the model of its own record (at evaluation). Before the frame
    (`renderFrame`) and after layout (`layoutFrame`, up to 3 passes together with virtual list windows)
    `ExpressionBindings::update` first transfers user edits to the sources of two-way bindings
    (variable or record field), then compares the dependencies of each binding with the remembered ones — property strings
    and version numbers of variables and models, without calculating XPath — and evaluates only the changed ones anew; writing
    into the node happens only on a new result. It is not necessary to embed notifications into each node change path (input, code,
    layout), and frames are drawn only on events anyway.
  - Owner — `ListBindings` (as list bindings): template copies receive expressions through the same `bind`,
    node deletion removes them through `forget`; `gran_snapshot` evaluates the expressions without a window.

- **Components** (`markup/component_*`). A library is a separate `<Library namespace>` file with `<Style>` and
  `<Component name>` (`<Param>`, own `<Style>`, the body is one element, `<Slot>`); a window includes it with `<xi:include>`
  at the end of `<Resources>`. Inclusion is done by the loader (`loadComponents`), not by the parser `processXInclude`: the `<xi:include>` itself
  stays in the window DOM, and saving does not paste the library in. The built-in schema knows `<Library>` as the second
  global element (a library file is checked by the same schema), `<Slot>` and `xs:any ##other` (lax) in the UI
  element group — a `<w:Card>` instance passes it as "an element of another namespace".
  - Schema: for each library an XSD is generated (`component_schema.cpp`, `targetNamespace` = namespace, `xs:import`
    without a location, the `gran-ui.xsd` types and groups), `Schema::fromDocuments` builds it together with the embedded one; it checks
    the window and all libraries (instances in bodies) at load time, in `appendMarkup` fragments and on every
    live edit (`documentSchema`). The schema lives in `UiDocument::components` (`ComponentSet`: the libraries with their DOM,
    the `ComponentSpec` declarations, the common schema).
  - Expansion (Prototype pattern): an instance → a body copy in its own Beresta document (`makeShadow`) with substitutions
    (`Substitution`): a whole `{$parameter}` is the instance value as is, inside an expression — an XPath value
    (a string, a number, `true()`, the instance expression in brackets, a mixed text through `concat`), local ids →
    "prefix.id" (and in `$id.property`, `labeledBy`, `popup`), `style` → the name in the document style set (own styles
    are registered as `_N.Component.Style`, library styles — `_N-Style`), the host attributes go over the root.
    The copy is built by the usual `buildNode`, so bindings, events, commands, lists and accessibility work
    without special cases. The expansion root refers to the instance element (`domElement`) and owns the copy
    (`ui::Node::expansions` — declared before `children`, so the children are destroyed before the copy).
  - Slots: `BuildContext::scope` — the expansion chain (`ComponentScope`: component, instance, outer expansion).
    The slot content is built in the `outer` scope — where the instance is written: its elements are real elements
    of the window DOM (editing them works as usual), a nested `<Slot>` forwards a slot of the outer component. Recursion
    (the component is already in the chain) is caught by `expandComponent`.
  - Live document: editing an instance attribute is an ordinary node rebuild; body elements live in the copy, so
    `editMarkupAttribute` rejects them (the element owner is not the window DOM). Partial reload requires equal
    libraries (`sameComponents`); the instance is compared as any element: an equal subtree keeps the expansion,
    otherwise the instance is rebuilt. `Application` watches library files together with the main one (`reload_libraries.cpp`).

## 4. Rendering

D3D11 is an additional FL11_0/SM5 backend using the same scene structures and HLSL (DXBC through fxc).
It binds the atlas, ramp and backdrop to fixed SRV registers and splits segments into ordered batches
when the image changes. Both APIs support snapshots, WARP and Kawase backdrops.
`AUTO` first creates D3D12 and tries D3D11 if creation fails; an explicit API choice is retained.
XML reloading replaces the tree at a loop boundary and disconnects old input and UIA references while
keeping the window and GPU backend. See [API](api.en.md).

- **One "uber" shader and one instanced call**: square is built from `SV_VertexID`, data — from
  `StructuredBuffer<GpuInstance>`; instance type (rectangle, shadow, glyph, image, background) — field `kind`.
  Textures — "unlimited" array of descriptors (index in the instance), so a texture change does not break the batch.
- **SDF rounded rectangle** with separate corner radii, border, fill with color or gradient
  (linear, radial, conical; ramp on 256 texels in the ramp atlas), analytical anti-aliasing (without MSAA).
- **Shadows** `box-shadow` (outer and inner): Gaussian blur of a rounded rectangle in a closed form.
- **Background blur**: copy of the target → double Kawase (down/up) → `BACKDROP` instance takes the blurred texture
  with its mask and tinting.
- **Clipping** with rounded rectangles — SDF in the pixel shader by index from the clipping table,
  without state change and without scissor.
- **Text** — coverage atlas R8: glyph is rasterized on CPU with precise integral area under quadratic curves
  at the required size and with subpixel shift (¼ px), as in browsers; large and scalable text — later SDF.
- **Color and blending** — in sRGB, premultiplied alpha (as in browsers).
- **In-flight frames** — 2, a ring upload buffer, deferred deletion; rules — [docs/development.en.md](development.en.md#gpu-usage-rules).

## 5. Layout (Predictable Model)

- Block: `margin` (no collapse) → `border` → `padding` → content. `width`/`height`: `auto`, pixels, `%`.
  `minWidth`/`maxWidth`/`minHeight`/`maxHeight` are always respected.
- `Stack`: `orientation`, `spacing`, `justify` (`start|center|end|space-between|space-around|space-evenly`),
  `align` (`start|center|end|stretch`), children `grow`, `shrink`, `alignSelf`; `wrap`.
- `Grid`: `rows`/`columns` — list of tracks (`48`, `auto`, `1fr`, `minmax(100,1fr)`), `rowSpacing`/`columnSpacing`,
  children `gridRow`, `gridColumn`, `gridRowSpan`, `gridColumnSpan`.
- `Canvas`: `left`/`top`/`right`/`bottom` for children.
- A growing child (`grow`) of a stack with finite space takes all of it already during measurement: cross dimensions (text height with line breaks) are measured by final lengths, not by base lengths.
- `Popup` is taken out of the flow (like `position: fixed`): measured by the window and placed at the anchor (below it, above it if there is not enough space, at the point — with reflection inside the window).
- One measurement pass + one layout pass; measurement cache by constraints.

- **Local relayout.** `markDirty(node, DIRTY_LAYOUT)` raises the flag to all ancestors, so a node without the flag is an
  unchanged subtree. Its measurements (the two last available sizes) live between passes, and during placement a branch
  of the same size is not recomputed: at the same place — nothing, at a new one — the subtree frames are only shifted.
  Editing one record in a list of 10,000 costs ~1 ms instead of ~60 ms. The stack main axis sums are in `double`,
  and free space within the rounding error counts as zero: otherwise a long stack would "shrink" by pixel fractions,
  and every item would be measured anew.
- **Virtual lists** (`virtual="true"`, the `binding/` module). Only the record window near the visible scroll area has
  template copies; the records before and after the window are replaced by two `Box` spacers sized "the record count ×
  the average size". After a layout `ListBindings::updateVirtual` fits the window (an overlapping one — at its edges, the
  other copies live on), and one more style pass and layout run in the same frame (`app::layoutFrame`, at most three).
  The spacers change only on a difference above half a pixel — the average jitter does not relayout the list.

## 6. Performance (Goals)

| Metric | Goal |
|---|---|
| Cold start of a window with a form of 200 elements | < 30 ms to the first frame |
| Parsing + schema + style + layout of 1,000 elements | < 2 ms |
| Frame during animation (CPU) | < 1 ms, 1 draw call |
| Process memory with an open window | < 20 MB (without the driver); AMD driver for D3D12 takes ~92 MB, D3D11 — ~34 MB |
| Idle | 0 % CPU, 0 frames |

## 7. Stages

1. Skeleton, rules, build (Makefile + CMake), tests, message catalog.
2. Style values, XSD 1.1 schema with assertions, loading markup into the UI tree.
3. Cascading styles, Stack/Grid/Canvas layout.
4. Drawing list, instances, Null-backend.
5. D3D12 + Win32: window, shader, shadows, gradients, clipping; off-screen snapshots in PNG.
6. Text: TrueType, rasterization, atlas, kerning, line breaks.
7. Input: hover/pressed/focus, transitions, input field, scrolling.
8. PNG, images; background blur.
9. Vulkan 1.0 + X11 on Linux (implemented): GPU/Mesa CPU, fixed descriptors, two frames,
   deferred release, swapchain/offscreen/readback and shared HLSL/Kawase. SDK is build-only; macOS is deferred.
10. Measurements against Chromium/Electron and WebView2.

