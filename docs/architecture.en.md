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
        │   → ComputedStyle (POD) + transitions (animation/)
        ▼
 layout/   Stack (subset of flexbox), Grid (px/auto/fr/minmax, span), Canvas (absolute),
        │   text (text/: TrueType font, kerning, line breaks) → LayoutBox (x, y, w, h)
        ▼
 render/   rendering list → GpuInstance instances (rectangle SDF, shadow, glyph, image,
        │   background blur) + clip table + glyph atlas + gradient ramps
        ▼
 rhi/      D3D12 | Vulkan | Null (recording for tests): one instanced call per segment,
           segment changes only during background blur pass (glassmorphism)
```

Retained mode: each stage is recalculated only by dirty flags (`STYLE_DIRTY` → `LAYOUT_DIRTY` → `PAINT_DIRTY`). No changes and no animations — no frame, the thread sleeps waiting for OS events.

## 2. Modules (include/ and src/ are mirrored)

| Module | What's Inside |
|---|---|
| `core/` | `GranException` + `ErrorCode`, message catalog `i18n` (ru, en), logger |
| `utils/` | UTF-8, number and list parsing, geometry (`Vec2`, `Rect`, `Affine2D`), color |
| `markup/` | loading through Beresta, built-in schema `gran-ui.xsd`, building `ui::Node` |
| `style/` | value parsing (color, length, margins, radii, shadows, gradients, transitions), cascade, theme |
| `ui/` | UI node, element types, states, traversal |
| `layout/` | Stack, Grid, Canvas, text measurement, scrolling |
| `text/` | TrueType/TTC parsing, glyph rasterization (exact coverage area), atlas, kerning, line breaks |
| `image/` | inflate (zlib) and PNG — custom implementations |
| `animation/` | property transitions, curves (`cubic-bezier`, ease, linear, steps), frame clock |
| `input/` | hit-test (considering transform), focus, event routing, text editing and selection, clipboard |
| `a11y/` | accessibility: roles, names, tree navigation (neutral); UI Automation provider in `platform/win32/uia_*` |
| `render/` | rendering list, instance packing, clips, segments, glyph cache |
| `rhi/` | backend interface, `null/`, `dx12/`, `vulkan/`, generated shaders `shaders_gen/` |
| `platform/` | window and events: `win32/`, `x11/` (then `cocoa/`) |
| `app/` | application: event loop, action binding (`onClick="name"` → C++ function) |

## 3. Markup and Schema

- Root — `<Window>`; resources — `<Resources>`: `<Theme>` (named colors and sizes), `<Style name=…>`
  with nested states `<Hover>`, `<Pressed>`, `<Focused>`, `<Disabled>`.
- Elements v1: `Stack`, `Grid`, `Canvas`, `Box`, `Text`, `Image`, `Button`, `Input`, `Scroll`; later added
  `CheckBox`, `Slider`, `TextArea` (multiline field: `rows`, `submitKey="ctrlEnter|enter"`, grows to `maxHeight`
  and its own scrolling), `Span` (formatted text segment inside `Text`: own font, color, `background`,
  `textDecoration`, link `onClick`), `Popup` (popup layer: hidden until `Application::showPopup`, out of the flow,
  above everything; Esc and clicking outside close it) and `MenuItem` (menu item in `Popup`). The `tooltip` attribute on any
  element — tooltip after 0.6 seconds; right-click on text and fields — built-in context menu.
- Attribute names — camelCase and identical for all elements (`gridRow`, `borderRadius`, `justify`, `align`).
  Differences from the concept draft: `grid-row` → `gridRow`; `alignment`/`verticalAlignment` →
  `justify` (main axis) and `align` (cross axis) — they do not depend on the orientation and are not confused.
- Built-in XSD 1.1 schema in the library checks the document **during parsing** (`ParserOptions::schema`):
  - simple types with patterns: color (`#rgb`, `#rrggbbaa`, `rgb()`, `hsl()`, `$themeName`), length, Grid track list;
  - `xs:assert` for interface constraints: `minWidth ≤ maxWidth`, Grid row/column number less than the number of tracks,
    `Button` has text or content, `Scroll` contains exactly one element, etc.;
  - `xs:unique` on `id` in the window, `xs:key`/`xs:keyref` — links `style="…"` only to declared styles.
  An error comes with the position and path before the window is created.

## 4. Rendering

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
9. Vulkan (Vulkan SDK is needed) + X11; then macOS (MoltenVK + Cocoa).
10. Measurements against Chromium/Electron and WebView2.

