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
9. [Lists from data](#lists-from-data)
10. [Pop-up Layers](#popup-layers)
11. [Embedded browser: WebView](#embedded-browser-webview)
12. [Errors and Message Language](#errors-and-message-language)

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
| `WebView` | A system browser page (optional, see [below](#embedded-browser-webview)) | `source` or HTML in the tag, `onMessage` |
| `ItemTemplate` | The look of one list record inside a container with `items` | exactly one element |

Containers (`Stack`, `Box`, `Grid`, `Canvas`, `Button`, `Popup`) accept `items="model"` together with
`<ItemTemplate>` — see [Lists from data](#lists-from-data).

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
| `appendMarkup(parent, "<Stack>…</Stack>")` | Insert a markup fragment (the same schema, the document styles and theme) |
| `model(name)` / `itemOf(node)` | The list model of `items="name"` / the record a node belongs to |
| `onWebMessage`, `postWebMessage`, `navigate`, `loadHtml` | The embedded `<WebView>` browser |
| `focus(node)` | Pass keyboard focus |
| `showPopup(layer, anchor)` / `showPopupAt(layer, point)` / `hidePopup(layer)` | Popup layers |
| `post(task)` | Execute a function in the UI thread (safely from any thread) |
| `run()` | Event loop until the window is closed; returns the exit code |

Node `gran::ui::Node` (`gran/ui/node.hpp`) stores the element kind, `id`, the content `content` (`text`, `value`,
`checked`, `placeholder`…) and computed style. After changing the content from code, the node is marked
`ui::markDirty(node, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT)`: the engine re-lays out and repaints only what is needed.

**Templates.** A repeated block of the same look is better described as a list from data (the section below).
If a block is assembled in a more complex way (like a Markdown answer in `examples/chat/`), it can be inserted as a markup
fragment with `appendMarkup`, or a hidden sample can be copied with `ui::cloneTree` and inserted with `append`.

**Background threads.** Access to nodes can only be made from the UI thread. A network or computation thread
passes the result through `app.post(task)` — the task will be executed in the event loop, and the window will
wake up itself.

## Lists from data

The markup describes the appearance of one record, the data lives inside the engine's model — the code does not need to build nodes:

```xml
<Resources>
  <Data name="contacts">                       <!-- initial data (can also be from code) -->
    <Item name="Anna" mail="anna@example.org"/>
    <Item name="Boris" mail="boris@example.org"/>
  </Data>
</Resources>
...
<Stack items="contacts" spacing="8">
  <Text>Contacts</Text>                       <!-- own children remain first -->
  <ItemTemplate>
    <Stack style="row" tooltip="{mail}" onClick="openContact">
      <Text fontWeight="600">{name}</Text>
      <Text>Email: <Span color="#2563eb">{mail}</Span></Text>
    </Stack>
  </ItemTemplate>
</Stack>
```

```cpp
gran::data::ListModel& contacts = app.model("contacts");
contacts.append({{"name", "Vera"}, {"mail", "vera@example.org"}});   // + copy of the template
contacts.set(0, "name", "Anna S.");                                  // only this text changes
contacts.remove(1);                                                  // one copy is removed

// in the openContact action handler:
gran::ItemRef item = app.itemOf(node);              // item.model == &contacts, item.index — record number
```

- Substitutions `{field}` work in `text`, `placeholder`, `source`, `tooltip`, and in tag text (including
  in `Span`). `{{` and `}}` — the braces themselves; if the field does not exist in the record, it gives an empty string.
- Model editing changes only the affected copies: insertion and deletion — their own nodes, field change — only texts
  with substitutions. Focus, selection, and scrolling of other records are preserved.
- Record — strings (`gran::data::Record`: field name → text); the format of values is determined by the application.
- `<Data>` replaces the model's contents on loading; the model can be populated before loading the markup.
- Lists can be nested (template inside template), but the nested list is linked to the model by name — common
  for all records.
- The model changes only in the UI thread; the background thread passes data through `app.post`.
- `gran_snapshot` builds lists from `<Data>` without a window — the list frame is checked without the program
  (example — `samples/lists.xml`).

Markup fragment during operation — `app.appendMarkup(parent, "<Stack>…</Stack>")`: one root element,
validation by the same schema, styles `style="…"` and theme values `$name` — from the document. Error — `MarkupException`
with position inside the fragment; the tree does not change.

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

## Embedded browser: WebView

Gran UI itself does not contain a browser and does not use it for the interface: the window, text, fields, and lists are drawn
by the engine. If the program needs a web page (help, report, login via a website, ready-made web widget), it is displayed
by the `<WebView>` element — **hybrid**: the entire window is drawn by the engine, and inside the `<WebView>` rectangle the page
is shown by the system browser.

```xml
<WebView id="page" source="https://example.org" height="320" onMessage="fromPage"/>
<WebView id="help" source="help/index.html" grow="1"/>          <!-- file next to the markup -->
<WebView id="card" height="200"><![CDATA[<h2>HTML directly in markup</h2>]]></WebView>
```

```cpp
app.onWebMessage("fromPage", FromPage{&app});   // void(ui::Node& webView, const std::string& text)
app.postWebMessage(*app.find("page"), R"({"theme":"dark"})");
app.navigate(*app.find("help"), "help/faq.html");
```

In the page:

```js
window.chrome.webview.postMessage("ready");                            // → onMessage handler
window.chrome.webview.addEventListener("message", e => show(e.data));  // ← postWebMessage
```

How it works:

- **Lazy creation.** The browser is created only when `<WebView>` first appears on the screen. A window without
  visible `WebView` does not spend any memory or time on the browser startup.
- **Separate process.** The page lives in the system browser processes, not in the program's process; its crash
  does not bring down the interface.
- **Only strings.** The bridge — string messages in both directions (format, for example JSON, is chosen by the program).
  C++ objects are not passed to the page script; developer tools (F12) are enabled only
  with `AppOptions::debugLayer`.
- **Size — from style.** The page does not have its own size: set `width`/`height` or `grow`.
  The frame and padding of the element are visible around the page; the part that goes beyond the `Scroll` edge or `overflow="hidden"`
  is clipped.
- **Limitations.** The page lies on top of what the engine draws: a popup layer (`Popup`, tooltip) over
  a `WebView` ends up under the page, and `borderRadius`, `opacity` and `transform` do not apply to the page itself.
  The composition mode through DirectComposition — the next step — removes these limitations.

**Platforms and build.** Windows — WebView2 (Edge, Chromium); macOS (WKWebView) and Linux (WebKitGTK) — in plans.
Support is enabled at build time: `make WEBVIEW2_SDK=<package>/build/native` or
`cmake -DGRAN_WEBVIEW2_SDK=<package>/build/native` (NuGet package `Microsoft.Web.WebView2`). Without this parameter
the engine is built as before, and `<WebView>` remains a white background.

A program with `WebView` needs two things: `WebView2Loader.dll` next to the exe (the build places it next to the programs;
it is only searched for in the program directory and in System32) and the WebView2 Runtime environment (already present in Windows 11, installed separately in Windows 10). The loader is connected via `LoadLibrary` on the first display of `WebView`, so programs without `WebView` do not know about it. If the loader or environment is not present, `WebView` remains a background, the reason is output to stderr, the program continues to work. Browser data (cache, cookies) are stored in
`%LOCALAPPDATA%\GranUI\<program name>\WebView2`. Example — `samples/web_view.xml`.

## Errors and Message Language

All engine errors are exceptions `gran::GranException` (`gran/core/gran_exception.hpp`) with the code `ErrorCode`:

- `MarkupException` — markup or schema error: `getLine()`, `getColumn()`, path to the element in the text;
- `GpuException` — GPU error with the original `HRESULT` code.

Message texts are stored in the message catalog (Russian and English). The language is changed by calling
`gran::i18n::setLanguage("en")` (`gran/core/i18n.hpp`).

