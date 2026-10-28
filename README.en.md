# Gran UI

[Русский](README.md) · **English**

**Gran UI** is a user interface engine written in **C++17**: the window is described using XML markup, styled with CSS-like syntax, and rendered directly onto the graphics card via **Direct3D 12/11 on Windows or Vulkan on Linux**. The goal is to achieve the look and feel of HTML + CSS (gradients, shadows, rounded corners, frosted glass, smooth transitions) without a browser, without a garbage collector, and without third-party dependencies: startup in milliseconds and 0% CPU usage when idle. When a web page is needed after all, it can be embedded with the `<WebView>` element — the system browser appears only inside it and only while it is visible.

The XML markup is parsed and validated by the XML library [Beresta](https://github.com/ErmaleyAlekov/beresta) by the same author: the XSD 1.1 schema is built into the engine, and any markup error is visible with the line, column, and path even before the window is created.

The name comes from the Russian word *gran* — a facet: the interface is built from clear-cut blocks, like a crystal. In Russian the engine is called **Грань UI**.

![Gran UI Showcase](docs/images/showcase.png)

```xml
<Window title="Hello" width="420" height="200" background="#0f1117" color="#e6e8ee">
  <Stack padding="24" spacing="12">
    <Text fontSize="20" fontWeight="600">Hello, Gran UI</Text>
    <Input id="name" placeholder="What is your name?"/>
    <Button text="Greet" onClick="greet" background="linear-gradient(135deg, #6d5dfc, #b05cff)"/>
  </Stack>
</Window>
```

```cpp
gran::Application app;
app.loadFile("hello.xml");
app.onAction("greet", GreetAction{&app});   // onClick="greet" → C++ function
return app.run();
```

> **Status:** Under development. Windows 10/11 uses Direct3D 12/11 and Win32; Linux uses Vulkan 1.0 and X11
> (Wayland sessions require XWayland). Software rendering uses WARP on Windows and Mesa lavapipe on Linux.
> macOS is currently unsupported. The API may change.

## Linux, including Russian distributions

Linux uses Beresta, C++17 and the system graphics stack: the Vulkan loader, a Vulkan driver and X11.
No SDL, GLFW, Qt, GTK, FreeType or Fontconfig is required. A CPU device such as Mesa lavapipe is selected
when no suitable GPU is available; `--software` forces CPU rendering. The Vulkan SDK is only needed for
build headers/tools, never on the target machine. The loader and a hardware or Mesa software driver
must be installed by the OS; the engine does not download drivers.

This build path also targets Astra Linux, ALT and RED OS with C++17, Vulkan 1.0 and X11/XWayland.
Each specific release, architecture and certified package set requires validation in its own environment.
SPIR-V is embedded, so normal builds do not require a shader compiler.

```bash
cmake -S . -B build -DGRAN_BERESTA_DIR=../beresta
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/bin/gran_gallery samples/showcase.xml --vulkan
./build/bin/gran_snapshot samples/glass.xml glass.png --software
```

Building requires Vulkan SDK headers (or the system Vulkan development package) and system X11 headers.
Gallery, snapshots and the C ABI support Linux. The chat example currently uses WinSock and remains
Windows-only. On Linux `<WebView>` is a plain block without a browser; AT-SPI is not yet implemented.
See [Linux build and validation](docs/building.en.md#linux).

---

## Documentation

- [Guide: Markup, Styles, and API](docs/guide.en.md)
- [C++ API Reference and XML Reloading](docs/api.en.md)
- [C ABI and Python, C#, Java bindings](docs/bindings.en.md)
- [Rendering Tests and WARP Baselines](docs/render_testing.en.md)
- [Building and Integration](docs/building.en.md)
- [Engine Architecture](docs/architecture.en.md)
- [Project Structure, Tests, and Coding Rules](docs/development.en.md)
- [Security: what counts as a vulnerability and how to report it](SECURITY.en.md)

---

## Advantages

### Lightweight
- **No browser, no JavaScript, no garbage collector.** The engine and Beresta are statically linked.
  Windows also uses a static C++ runtime; Linux uses the system runtime and system Vulkan/X11.
  The Windows chat example weighs about **3 MB** after `strip`.
- **A browser only on demand.** An embedded web page (`<WebView>`) is an optional part: it is enabled at build time, the system browser is created when the element is first shown, and programs without a `WebView` never load it.
- **The engine's own memory with an open window is about 5 MB**; the rest is taken by the graphics card driver.
- **Fast startup:** the first frame of the 47-element showcase takes tens of milliseconds (parsing with the schema 4–8 ms, styles and layout 5–16 ms, scene about 9 ms).
- **0% CPU when idle:** a frame is drawn only when something changes or there is an animation.
- **No third-party dependencies:** nothing to update, nothing to break in someone else's package.

### Strict about markup
- **Each file is checked against the XSD 1.1 schema right during parsing** — before the window is created. HTML silently forgives errors and draws whatever comes out; Gran UI will not load a window with an error.
- **It catches what on the web shows up only on screen:** a typo in an element or attribute name, an incorrect color or length, a reference to an undeclared style, a duplicate `id`, `minWidth` greater than `maxWidth`, a `Grid` row number beyond the tracks, `Scroll` with two children, a button without text.
- **All errors at once, each with its location:**
  `line 21, column 7: /Window/Stack/Grid/Box[2]: attribute "transformOrigin": value "center" does not match the pattern …`
- **Unified rules:** the same attribute names for all elements (camelCase), a predictable box model without margin collapsing, `minWidth`/`maxWidth` are always respected.

### Secure
- **Markup is only data.** It has no scripts or expressions: `onClick`, `onChange`, `onSubmit` are just action names, and only what the program itself has bound to C++ functions runs. An unknown action does nothing.
- **An interface without a browser or network:** markup, fonts, and images are read only from local files.
  Web content is possible only inside an explicit `<WebView>`: the page runs in a separate process of the system
  browser and talks to the program only with strings — C++ objects are not available to the page script.
- **Secure XML parsing:** Beresta does not load external DTDs or XInclude, limits entity expansion (protection against "billion laughs") and nesting depth (no more than 256 levels).
- **Custom parsers check every byte:** PNG — CRC of each chunk, image size limits (16,384 pixels per side, 32 Mpixels), inflate with a limit on the result size; TrueType — boundary checks at each read and a limit on the nesting of composite glyphs. A damaged file gives an error, not an out-of-bounds memory access.
- **Careful work with the graphics card:** each write to the GPU buffer is checked against capacity, the resource is deleted only when the GPU is done with it, the D3D12 debug layer is on in frame checks.

---

## Features

### Markup and Styles
- **Elements:** `Window`, `Stack`, `Grid`, `Canvas`, `Box`, `Scroll`, `Text` with `Span`, `Image`, `Button`,
  `Input` (including password), `TextArea`, `CheckBox`, `Switch`, `RadioButton`, `Slider`, `ProgressBar`, `ComboBox`, `Tabs`, `Popup`,
  `MenuItem`, `Dialog`, and `WebView` (optional).
- **Lists from data:** `items="model"` + `<ItemTemplate>` with `{field}` placeholders; the data comes from `<Data>`
  or from a model filled in code, and a model edit changes only the affected records. Markup fragments at run time
  (`appendMarkup`) are checked by the same schema.
- **XSD 1.1 schema validates markup during parsing:** value types (colors, lengths, Grid tracks),
  `xs:assert` assertions (`minWidth ≤ maxWidth`, Grid row number less than the number of tracks, button has text),
  unique `id` and references only to declared styles (`xs:key`/`xs:keyref`).
- **Styles in CSS syntax:** colors (`#rgb`, `rgba()`, `hsl()`, names), `linear`/`radial`/`conic` gradients,
  `boxShadow` (outer and inner), separate corner radii, `transform`, `opacity`, `backdropBlur`.
- **Cascading:** defaults ← theme ← named `Style` (with `basedOn` inheritance) ← attributes ← states
  `Hover`, `Pressed`, `Focused`, `Checked`, `Disabled`. Theme tokens are `$name`.
- **Transitions and animations:** `transition` with curves `ease`, `linear`, `cubic-bezier()`, `steps()`; keyframe
  animations `<Keyframes>` + `animation` (repeats, `infinite`, `alternate`, delay, fill), state-triggered as well
  (`<Hover animation="…"/>`); `setTimeout`/`setInterval` timers on the UI thread.

### Layout
- `Stack` — a subset of flexbox: `justify`, `align`, `grow`/`shrink`, `wrap`, `alignSelf`.
- `Grid` — tracks `48`, `auto`, `1fr`, `minmax(100, 1fr)`, cell merging.
- `Canvas` — absolute positioning; `Scroll` — scrolling with scrollbars and mouse wheel.
- Predictable block model: `margin` without collapsing, `minWidth`/`maxWidth` are always respected.
- Local relayout: only the changed branch is recomputed, unchanged boxes take their sizes from the cache or are only
  shifted (editing a record in a list of 10,000 — about 1 ms).
- Virtual lists (`virtual="true"`): only the visible records have nodes — 100,000 records are built in a fraction
  of a millisecond.

### Text
- Custom TrueType and TTC font parsing: `cmap`, `glyf`, composite glyphs, kerning `kern` and `GPOS`.
- Rasterization by exact coverage area with subpixel offsets, like in browsers; glyph atlas on the GPU.
- Word wrapping, justified alignment, ellipsis, font replacement for CJK.
- Mouse and keyboard selection, clipboard, context menu, styled text (bold, italic, code, links).

### Rendering
- One shader and one draw call per frame: each rectangle, shadow, and glyph is an instance in a common buffer.
- Rounded rectangles and clipping through SDF, analytical anti-aliasing without MSAA.
- Frosted glass: background blurring with double Kawase.
- Two frames in flight, a ring upload buffer, deferred resource deletion; D3D12 debug layer and GPU-Based Validation.
- Retained mode: only changed parts are recalculated; no changes and no animations — no frames.

### Application
- Event loop, actions from markup (`onClick`, `onChange`, `onSubmit`) are linked to C++ functions.
- Focus and Tab, input fields, pop-up menus with submenus, drop-down lists, modal dialogs with dimming,
  and tooltips (`tooltip`); menus open from the keyboard too.
- A hybrid with the web: `<WebView>` shows a system browser page (Windows — WebView2) inside the engine window;
  the browser is created lazily, the page lives in a separate process, the bridge to the program is string messages.
- Accessibility: Microsoft UI Automation provider — screen readers see buttons, fields, checkboxes, sliders, and texts
  and control them by themselves (press, toggle, select, type text, set a number); names — `accessibleName` and `labeledBy`.
- Several windows (`openWindow`), `onClose` with confirmation, the system open, save and folder dialogs,
  dragging files from Explorer onto an `onDrop` zone with a highlight.
- Vector icons `<Icon>`: an SVG path or an `.svg` file, crisp at any scale, colored like text.
- PNG and JPEG (progressive too) — custom decoders and custom inflate. Error messages are in Russian and English.

### Engineering Quality
- **No third-party dependencies:** Beresta, the standard C++17 library and the system graphics SDK/driver.
- **Build:** Makefile with auto-dependencies and CMake; MinGW-w64, MSVC, GCC and Clang.
- **Testing without a window:** `gran_snapshot` renders the markup off-screen to PNG — the frame can be compared to the reference.
- **Readable code:** short functions and small files, detailed bilingual comments (Russian and English).

---

## Examples

| Program | What It Shows |
|---|---|
| `gran_gallery [markup.xml]` | Live window with any markup from `samples/` (the showcase by default) |
| `gran_snapshot <markup.xml> <frame.png> [width height [scale]]` | Frame without a window in PNG |
| `gran_chat [--model name]` | Chat with a local model through [Ollama](https://ollama.com): streaming response, Markdown, multiline field |

![Frosted glass](docs/images/glass.png)

---

## Who Does What

Gran UI is made by a person together with two language models. The decisions are made by the person,
and the models work on the author's assignments. Every change is checked by unit tests, off-screen frames and runs of live windows.

| Participant | Role |
|---|---|
| **Ermakov Aleksei Aleksandrovich** — author and rights holder | The main idea and concept of the engine, setting the tasks and making the decisions. Creating and editing the code generation rules for both models, including rules for working with the graphics card. Checking the code in a debugger, finding bugs. Checking the appearance and behavior of live windows. Author of the XML library Beresta the engine is built on. |
| **Claude** (Anthropic), assistant in Claude Code | Architecture and module design. Implementation of most of the code: markup loading and the schema, styles and cascade, layout, fonts and text, scene and Direct3D 12 backend, shaders, input, animations, accessibility, examples. Diagnosing bugs, building, documentation. Reviewing the local model's code. |
| **Qwen2.5-Coder 14B** (Alibaba), local model | Simple boilerplate code from a ready assignment: unit tests, headers with declarations, English translations of the documentation. |

The rights to the program belong to the rights holder (see [LICENSE](LICENSE)).

---

## License

Gran UI is **proprietary software**, all rights belong to Ermakov Aleksei Aleksandrovich.
Using, copying, modifying and distributing it is allowed only under a written agreement with the rights holder;
the terms are in the [LICENSE](LICENSE) file. Beresta is a separate open project under the MIT license.

