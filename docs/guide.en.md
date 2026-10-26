# Gran UI — Guide

[Русский](guide.md) · **English**

← [Back to Description](../README.md)

## Table of Contents

1. [Quick Start](#quick-start)
2. [Window and Resources](#window-and-resources)
3. [Elements](#elements)
4. [Layout](#layout)
5. [Style Values](#style-values)
6. [Styles, Themes, and States](#styles-theme-and-states)
7. [Text and Fonts](#text-and-fonts)
8. [Actions and API](#actions-and-api)
9. [Pop-up Layers](#popup-layers)
10. [Errors and Message Language](#errors-and-message-language)

## Quick Start

`hello.xml`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<Window title="Hello" width="420" height="220" background="#0f1117" color="#e6e8ee" fontSize="14">
  <Stack padding="24" spacing="12">
    <Text fontSize="20" fontWeight="600">Hello, Gran UI</Text>
    <Input id="name" placeholder="What is your name?"/>
    <Stack orientation="horizontal" spacing="12" align="center">
      <Button text="Greet" onClick="greet"/>
      <Text id="answer" color="#8b93a7"/>
    </Stack>
  </Stack>
</Window>
```

`main.cpp`:

```cpp
#include "gran/app/application.hpp"
#include <iostream>

// Button action: read the field and show the response
struct GreetAction
{
    gran::Application* app;

    void operator()(gran::ui::Node& button) const
    {
        gran::ui::Node* name = app->find("name");
        gran::ui::Node* answer = app->find("answer");
        (void)button;
        if (name == nullptr || answer == nullptr)
            return;
        answer->content.text = "Hello, " + name->content.text + "!";
        gran::ui::markDirty(*answer, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
    }
};

int main()
{
    try
    {
        gran::Application app;
        app.loadFile("hello.xml");
        app.onAction("greet", GreetAction{&app});
        return app.run();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
```

How to build it — see [docs/building.en.md](building.en.md#integration-with-your-program). Ready-made markup for experiments
is in `samples/`, it is opened with `bin/gran_gallery <file.xml>`.

## Window and Resources

The document root is `<Window>`: `title`, `width`, `height` (default 800 × 600), `minWidth`, `minHeight`,
as well as any styling attributes that children inherit (`background`, `color`, `fontSize`, `fontFamily`).
Inside: an optional `<Resources>` and exactly one root interface element.

```xml
<Window title="Window" width="960" height="620">
  <Resources>
    <Theme>
      <Token name="accent" value="#6d5dfc"/>
      <Token name="radius" value="16"/>
    </Theme>
    <Font family="Inter" source="fonts/Inter-Regular.ttf" weight="400"/>
    <Style name="card" background="#171a23" borderRadius="$radius" padding="20"/>
  </Resources>
  <Stack style="card">…</Stack>
</Window>
```

Markup is validated by the built-in XSD 1.1 schema (`schemas/gran-ui.xsd`) directly during parsing. Unknown
attribute, incorrect color, reference to an undeclared style, duplicate `id`, `minWidth` greater than `maxWidth` or row number
in `Grid` outside the tracks — error with line number and path before the window creation.

## Elements

| Element | Purpose | Own attributes |
|---|---|---|
| `Stack` | Row or column of children (subset of flexbox) | `orientation`, `spacing`, `justify`, `align`, `wrap` |
| `Grid` | Grid of rows and columns | `rows`, `columns`, `rowSpacing`, `columnSpacing` |
| `Canvas` | Absolute positioning of children | children have `left`, `top`, `right`, `bottom` |
| `Box` | Empty rectangle (background, border, shadow) | — |
| `Scroll` | Scroll exactly one child | `direction="vertical\|horizontal\|both"` |
| `Text` | Text, can contain `Span` segments | `textWrap`, `textOverflow`, `maxLines`, `textAlign` |
| `Span` | Formatted text segment inside `Text` | font, `color`, `background`, `textDecoration`, `onClick` (link) |
| `Image` | PNG image | `source`, `fit="fill\|contain\|cover\|none"` |
| `Button` | Button with text or content | `text`, `onClick` |
| `Input` | Single-line field | `value`, `placeholder`, `maxLength`, `password`, `readOnly`, `onChange`, `onSubmit` |
| `TextArea` | Multi-line field with wrapping and scrolling | `rows`, `submitKey="ctrlEnter\|enter"`, `maxHeight`, `onSubmit` |
| `CheckBox` | Checkbox | `text`, `checked`, `onChange` |
| `Slider` | Slider | `min`, `max`, `value`, `step`, `onChange` |
| `Popup` | Popup layer (hidden until shown) | — |
| `MenuItem` | Menu item in `Popup` | `text`, `onClick` |

Common attributes of all elements: `id`, `style`, `visible`, `enabled`, `tooltip`, size, margin, and styling attributes. Attribute names are camelCase and identical for all elements.

## Layout

Block model: `margin` (no collapsing) → `borderWidth` → `padding` → content. `width` and `height` are `auto`, pixels, or percentages; `minWidth`, `maxWidth`, `minHeight`, `maxHeight` are always respected. Spacing is written as in CSS: `padding="8"`, `"8 16"`, `"4 8 12 16"`.

**Stack.** `orientation="vertical\|horizontal"`, `spacing` — gap between children. `justify` distributes children along the main axis (`start`, `center`, `end`, `space-between`, `space-around`, `space-evenly`), `align` along the cross axis (`start`, `center`, `end`, `stretch`). Children have `grow`, `shrink`, `alignSelf`. `wrap="true"` wraps children to a new line.

**Grid.** Tracks are listed with commas: number (pixels), `auto`, `1fr`, `minmax(100, 1fr)`.

```xml
<Grid rows="auto, 1fr" columns="1fr, 1fr, 1fr" rowSpacing="20" columnSpacing="20">
  <Text gridColumnSpan="3">Full-width header</Text>
  <Box gridRow="1" gridColumn="0"/>
  <Box gridRow="1" gridColumn="1" gridColumnSpan="2"/>
</Grid>
```

Row and column numbers start from zero.

**Canvas.** Children are placed by `left`/`top`/`right`/`bottom`; `zIndex` changes the stacking order.

**Scroll.** Child larger than `Scroll` — scrollbars appear, the mouse wheel and thumb dragging work.

## Style Values

| What | Examples |
|---|---|
| Color | `#rgb`, `#rrggbb`, `#rrggbbaa`, `rgb(…)`, `rgba(…)`, `hsl(…)`, `transparent`, CSS names, `$token` |
| Background | color; `linear-gradient(135deg, #6d5dfc, #b05cff)`; `radial-gradient(…)`; `conic-gradient(…)` |
| Border Radius | `borderRadius="12"` or per corner `"12 12 0 0"` |
| Shadow | `boxShadow="0 10 30 rgba(0, 0, 0, 0.45)"`, multiple separated by commas, `inset` — inner |
| Transform | `transform="rotate(12deg) scale(1.05) translate(4, 0)"`, `transformOrigin` |
| Opacity and Blur | `opacity="0.8"`, `backdropBlur="24"` — blurring what is behind the element |
| Transition | `transition="background 0.15s ease, transform 0.1s cubic-bezier(0.2, 0, 0, 1)"` |
| Cursor | `cursor="pointer\|text\|default"` |

Lengths are in logical pixels: the engine automatically multiplies them by 1.5 on a screen with a 150% scale.
Colors and blending are in sRGB with premultiplied alpha, as in browsers.

## Styles, Theme, and States

Cascade, from weakest to strongest: engine defaults ← built-in theme (`schemas/default_theme.xml`) ← style
`style="name"` ← element attributes ← active states.

```xml
<Style name="primary" background="linear-gradient(135deg, #6d5dfc, #b05cff)" color="#ffffff">
  <Hover boxShadow="0 6 18 rgba(109, 93, 252, 0.45)"/>
  <Pressed transform="scale(0.98)"/>
  <Disabled opacity="0.5"/>
</Style>
<Style name="ghost" basedOn="primary" background="transparent" borderWidth="1"/>
```

- States: `Hover`, `Pressed`, `Focused`, `Checked`, `Disabled`. State change is smooth if the property
  has a `transition`.
- `basedOn` inherits another style. The default theme sets styles with element names (`Button`, `Input`,
  `TextArea`, `CheckBox`, `Slider`, `Scroll`, `Popup`, `MenuItem`, `Tooltip`) — a `Style` with the same name replaces them.
- Theme tokens (`<Token name="accent" value="#6d5dfc"/>`) are substituted into any value as `$accent`.
- `color`, `accentColor`, the font and the font size are inherited by children, as in CSS.

## Text and Fonts

- Font: `fontFamily`, `fontSize`, `fontWeight` (100–900), `fontStyle="italic"`, `letterSpacing`, `lineHeight`.
  Windows system fonts are found by family name; a custom font file is added in the resources
  `<Font family="…" source="path.ttf" weight="…" italic="…"/>`. If there are no glyphs in the font (Chinese, Japanese) —
  the engine takes a fallback system font by itself.
- `textWrap="wrap|nowrap"`, `textAlign="start|center|end|justify"`, `textOverflow="ellipsis"`, `maxLines`.
- Styled text — `Span` segments inside `Text`; spaces collapse across segment boundaries, line breaks go through them:

```xml
<Text lineHeight="1.6">Normal, <Span fontWeight="700">bold</Span>,
  <Span fontStyle="italic">italic</Span> and <Span style="link" onClick="open">link</Span>.</Text>
```

- `Text` text and fields are selected by mouse (double-click — word) and keyboard (Shift+arrows, Ctrl+A),
  copied by Ctrl+C and through the context menu by right-click. The `password="true"` field shows dots and does not allow copying.

## Actions and API

Attributes `onClick`, `onChange`, `onSubmit` contain the name of the action. The application associates the name with a function:

```cpp
app.onAction("save", SaveAction{&app});   // the function receives the node that triggered the action
```

`onClick` — click or Enter/Space on a button, menu item, link. `onChange` — change in a field, checkbox,
slider. `onSubmit` — Enter in `Input`, `submitKey` key in `TextArea`.

Class `gran::Application` (`gran/app/application.hpp`):

| Method | What it does |
|---|---|
| `Application(AppOptions)` | `debugLayer` — D3D12 debug layer, `vsync`, `exitAfter` — exit after N seconds |
| `loadFile(path)` / `loadString(markup)` | Load and validate markup, build the window |
| `onAction(name, function)` | Bind an action name from the markup to a C++ function |
| `find(id)` | Find a node by `id` (or `nullptr`) |
| `setStyle(node, name)` | Change the named style of a node |
| `append(parent, node)` / `remove(node)` | Insert a node (e.g., a copy of the template `ui::cloneTree`) / remove |
| `focus(node)` | Pass keyboard focus |
| `showPopup(layer, anchor)` / `showPopupAt(layer, point)` / `hidePopup(layer)` | Popup layers |
| `post(task)` | Execute a function in the UI thread (safely from any thread) |
| `run()` | Event loop until the window is closed; returns the exit code |

Node `gran::ui::Node` (`gran/ui/node.hpp`) stores the element kind, `id`, the content `content` (`text`, `value`,
`checked`, `placeholder`…) and computed style. After changing the content from code, the node is marked
`ui::markDirty(node, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT)`: the engine re-lays out and repaints only what is needed.

**Templates.** Repeated block (chat message, list item) is described in the markup as a hidden template with
`visible="false"`; the code copies it with `ui::cloneTree`, edits the text in the copy (`ui::findNode`) and inserts it with
`append`. This is how the `examples/chat/` example is structured.

**Background threads.** Access to nodes can only be made from the UI thread. A network or computation thread
passes the result through `app.post(task)` — the task will be executed in the event loop, and the window will
wake up itself.

## Popup Layers

```xml
<Button id="more" text="File ▾" onClick="more"/>
<Popup id="moreMenu">
  <MenuItem text="Open…" onClick="open"/>
  <MenuItem text="Export" enabled="false"/>
</Popup>
```

```cpp
// In the "more" action: show the menu under the button
app.showPopup(*app.find("moreMenu"), button);
```

`Popup` is taken out of the layout and drawn on top of everything. It appears under the anchor (above it if there is not enough space at the bottom), `showPopupAt` — at the point with reflection inside the window. Esc and clicking outside close the layer, arrows navigate through items, Enter selects. The `tooltip` attribute on any element shows a tooltip after 0.6 seconds.

## Errors and Message Language

All engine errors are exceptions `gran::GranException` (`gran/core/gran_exception.hpp`) with the code `ErrorCode`:

- `MarkupException` — markup or schema error: `getLine()`, `getColumn()`, path to the element in the text;
- `GpuException` — GPU error with the original `HRESULT` code.

Message texts are stored in the message catalog (Russian and English). The language is changed by calling
`gran::i18n::setLanguage("en")` (`gran/core/i18n.hpp`).

