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
7. [Animations and Timers](#animations-and-timers)
8. [Text and Fonts](#text-and-fonts)
9. [Actions and API](#actions-and-api)
10. [Events](#events)
11. [Commands and shortcuts](#commands-and-shortcuts)
12. [Undo and redo](#undo-and-redo)
13. [Lists from data](#lists-from-data)
14. [Binding expressions](#binding-expressions)
15. [Tabs, radio buttons, switches](#tabs-radio-buttons-switches)
16. [Pop-up Layers](#popup-layers)
17. [Embedded browser: WebView](#embedded-browser-webview)
18. [Accessibility](#accessibility)
19. [Platform: windows, files, icons](#platform-windows-files-icons)
20. [Errors and Message Language](#errors-and-message-language)
21. [Custom window title bar](#custom-window-title-bar)

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

## Custom window title bar

The OS frame remains the default: `titleBar="system"`. For a custom title bar, set `titleBar="custom"`
on `<Window>` and draw the title and command buttons using ordinary elements:

```xml
<Window title="Example" titleBar="custom" width="800" height="500" minWidth="400" minHeight="240">
  <Stack>
    <Stack orientation="horizontal" windowRole="drag" height="40" align="center">
      <Text>My application</Text>
      <Box grow="1"/>
      <Button windowRole="minimize" text="−"/>
      <Button windowRole="maximize" text="□"/>
      <Button windowRole="close" text="×"/>
    </Stack>
    <Text>Window content</Text>
  </Stack>
</Window>
```

`drag` uses native moving, double click, Aero Snap and the title bar context menu. Alt+Space opens
the system menu. Inputs, buttons, WebViews and elements with `onClick` inside a drag region remain
interactive. Edges and corners resize with DPI awareness; maximized content stays within the monitor
work area. `resizable="false"` disables resizing and maximizing. On Windows 11, hovering over the
maximize role shows Snap Layouts when enabled in the OS. DWM supplies shadows and rounded corners;
Windows 10 may retain square corners.

Command roles work without `onClick` and support keyboard and screen reader input. An explicit
`accessibleName` overrides the localized command name. When maximized, the maximize role restores
the window. To change its icon, use `<Maximized opacity="…"/>` on child elements after other states.
The complete example with two icons and a search field is `samples/custom_window.xml`:
`bin/gran_gallery samples/custom_window.xml --d3d11 --debug`.

## Window and Resources

The document root is `<Window>`: `title`, `width`, `height` (default 800 × 600), `minWidth`, `minHeight`, `onClose`,
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
| `Icon` | A vector icon: an SVG path or an `.svg` file, the color is `color` | `data`, `viewBox`, `fillRule`, `source` |
| `Image` | PNG or JPEG image | `source`, `fit="fill\|contain\|cover\|none"` |
| `Button` | Button with text or content | `text`, `onClick`, `popup` (the id of its menu or dialog) |
| `Input` | Single-line field | `value`, `placeholder`, `maxLength`, `password`, `readOnly`, `onChange`, `onSubmit` |
| `TextArea` | Multi-line field with wrapping and scrolling | `rows`, `submitKey="ctrlEnter\|enter"`, `maxHeight`, `onSubmit` |
| `CheckBox` | Checkbox | `text`, `checked`, `onChange` |
| `Slider` | Slider | `min`, `max`, `value`, `step`, `onChange` |
| `RadioButton` | Radio button: one of a group is checked | `text`, `value`, `checked`, `group`, `onChange` |
| `Switch` | Switch (a check box with another look) | `text`, `checked`, `onChange` |
| `ProgressBar` | Progress bar | `min`, `max`, `value` |
| `Tabs` | Tabs: a header strip and the selected page | `value`, `onChange`, `Tab` pages (`text`, `value`) |
| `Popup` | Popup layer (hidden until shown) | — |
| `MenuItem` | Menu item in `Popup`; a nested `Popup` is a submenu | `text`, `onClick`, `value` (for `ComboBox`) |
| `Dialog` | Modal dialog in the window center | `closable` |
| `ComboBox` | Drop-down list with the selected value | `value`, `placeholder`, `onChange`, `MenuItem` items or `items` |
| `WebView` | A system browser page (optional, see [below](#embedded-browser-webview)) | `source` or HTML in the tag, `onMessage` |
| `ItemTemplate` | The look of one list record inside a container with `items` | exactly one element |

Containers (`Stack`, `Box`, `Grid`, `Canvas`, `Button`, `Popup`, `Dialog`) and `ComboBox` accept `items="model"`
together with `<ItemTemplate>` — see [Lists from data](#lists-from-data).

Common attributes of all elements: `id`, `style`, `visible`, `enabled`, `tooltip`, `accessibleName`, `labeledBy` (see [Accessibility](#accessibility)), size, margin, and styling attributes. Attribute names are camelCase and identical for all elements.

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
| Animation | `animation="pulse 0.8s ease-in-out infinite alternate"` — by `<Keyframes>` keyframes |
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

- States: `Hover`, `Pressed`, `Focused`, `Checked`, `Disabled`, `DropTarget` (files are carried over an `onDrop` zone). State change is smooth if the property
  has a `transition`; a state can also start an animation: `<Hover animation="wiggle 0.4s"/>`.
- `basedOn` inherits another style. The default theme sets styles with element names (`Button`, `Input`,
  `TextArea`, `CheckBox`, `Slider`, `RadioButton`, `Switch`, `ProgressBar`, `Tabs`, `TabBar`, `TabHeader`, `Tab`,
  `Scroll`, `Popup`, `MenuItem`, `Dialog`, `ComboBox`, `Tooltip`) — a `Style` with the same name replaces them.
- Theme tokens (`<Token name="accent" value="#6d5dfc"/>`) are substituted into any value as `$accent`.
- `color`, `accentColor`, the font and the font size are inherited by children, as in CSS.

## Animations and Timers

**Transitions** (`transition`) smoothly change a property from its old value to a new one when the state or style changes. **Keyframe animations** play on their own — on load, when a state appears, infinitely, — like `@keyframes` in CSS.

```xml
<Resources>
  <Keyframes name="pulse">
    <Frame at="0%" opacity="1" transform="scale(1)"/>
    <Frame at="100%" opacity="0.35" transform="scale(0.8)"/>
  </Keyframes>
  <Keyframes name="appear">
    <Frame at="0%" opacity="0" transform="translate(0, 16)"/>
  </Keyframes>
  <Style name="card" background="#ffffff" padding="16" animation="appear 0.6s ease-out"/>
</Resources>
…
<Box width="28" height="28" borderRadius="14" background="#e11d48"
     animation="pulse 0.8s ease-in-out infinite alternate"/>
```

- `<Keyframes name>` are declared in `<Resources>` before styles. A `<Frame at="0%…100%">` frame holds only animatable properties: `background` (color), `borderColor`, `color`, `accentColor`, `opacity`, `transform`, `boxShadow`, `borderRadius`. A property that is not in the 0% or 100% frame takes its usual value there (so `appear` above sets only the start). `easing` on a frame is the curve of the segment up to the next frame.
- `animation="name duration [curve] [delay] [repeats|infinite] [direction] [fill]"`, multiple — separated by commas, `none` — no animation. Direction: `normal`, `reverse`, `alternate`, `alternate-reverse`; fill: `none`, `forwards` (hold the last frame), `backwards` (first frame during the delay), `both`. Unlike CSS, the name always comes first.
- Animation starts when the name appears in the node's style (load, `setStyle`, state like `Hover`), and is removed when it disappears. A finished one does not repeat while the name stays in the style; to play it again, change the node's style to one without it, and then back in the next frame.
- Properties driven by an animation are not animated by transitions. Animations of hidden nodes (`visible="false"` on the node or ancestor: closed layer, another tab) do not trigger redraws and are at their proper place at once when shown.
- An undeclared name in `animation` is a load error with a line number. Gradients and transformations of different forms (`rotate` → `scale`) are not mixed: the value changes abruptly.

**Timers** — functions that the event loop calls in the UI thread:

```cpp
gran::TimerId tick = app.setInterval(0.05, ProgressTick{&app});   // every 50 ms
app.setTimeout(2.0, ShowHint{&app});                              // once after 2 seconds
app.clearTimer(tick);
```

- After triggering, the frame is redrawn; in idle, the thread sleeps until the deadline of the nearest timer. Accuracy — frame (around 16 ms with vsync) or event loop wake-up; period no less than 1 ms.
- A periodic timer calls the same function-object: its fields (e.g., counter) are saved between triggers. Missed triggers (thread was busy) are not caught up in a burst.
- A timer can be removed from its own handler; a timer set by a handler will not trigger until the next loop pass. Timers can only be set and cleared in the UI thread — a background thread does this through `post`.

Sample — `samples/animations.xml` (`gran_gallery samples/animations.xml`): rotation, pulse, color change, indeterminate progress, card appearance, hover animation; the progress bar and the seconds counter are driven by gallery timers.

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
switch, slider, choosing a radio button (the action of the checked one) or a tab (the action of `Tabs`). `onSubmit` — Enter in `Input`, `submitKey` key in `TextArea`.

Class `gran::Application` (`gran/app/application.hpp`):

| Method | What it does |
|---|---|
| `Application(AppOptions)` | `debugLayer` — D3D12 debug layer, `vsync`, `exitAfter` — exit after N seconds |
| `loadFile(path)` / `loadString(markup)` | Load and validate markup, build the window |
| `onAction(name, function)` | Bind an action name from the markup to a C++ function |
| `onEvent(name, function)` | A markup event handler (`onPointerDown="name"`…), the function gets the node and the `UiEvent` |
| `addEventListener(node, type, function, capture)` / `removeEventListener` | A node event listener from code |
| `dispatchEvent(node, type, string)` | A custom event: capture → target → bubbling; `false` — cancelled |
| `capturePointer(node)` / `releasePointer(node)` | The pointer capture until the button release |
| `find(id)` | Find a node by `id` (or `nullptr`) |
| `setStyle(node, name)` | Change the named style of a node |
| `append(parent, node)` / `remove(node)` | Insert a node (e.g., a copy of the template `ui::cloneTree`) / remove |
| `appendMarkup(parent, "<Stack>…</Stack>")` | Insert a markup fragment (the same schema, the document styles and theme) |
| `markupAttribute(node, name)` / `setMarkupAttribute(node, name, value)` / `removeMarkupAttribute` | A node attribute in the live markup: read / write with the schema check and a node rebuild / remove |
| `saveMarkup()` / `saveMarkupFile(path)` | The live window markup as XML (with the edits, insertions and removals) |
| `model(name)` / `itemOf(node)` | The list model of `items="name"` / the record a node belongs to |
| `onWebMessage`, `postWebMessage`, `navigate`, `loadHtml` | The embedded `<WebView>` browser |
| `focus(node)` | Pass keyboard focus |
| `showPopup(layer, anchor)` / `showPopupAt(layer, point)` / `hidePopup(layer)` | Popup layers |
| `showDialog(dialog)` / `hideDialog(dialog)` | Modal dialog |
| `selectOption(comboBox, value)` | Select a `ComboBox` item from code (without `onChange`) |
| `selectTab(tabs, value)` | Select a `Tabs` page from code (without `onChange`) |
| `setValue(node, number)` / `setChecked(node, yes/no)` | The number of a `ProgressBar`/`Slider`, the mark of a `CheckBox`/`Switch`/`RadioButton` (without `onChange`) |
| `post(task)` | Execute a function in the UI thread (safely from any thread) |
| `setTimeout(seconds, function)` / `setInterval(seconds, function)` / `clearTimer(id)` | Timers on the UI thread |
| `openWindow(path)` / `openWindowString(markup)` / `closeWindow(node)` | One more window / close the node window |
| `openFileDialog(options, multiple)` / `saveFileDialog` / `folderDialog` | The system file dialogs |
| `onFileDrop(name, function)` | Files dropped onto an element with `onDrop="name"` (nodes — the `drop` event, see «Events») |
| `run()` | Event loop until the main window is closed; returns the exit code |

Node `gran::ui::Node` (`gran/ui/node.hpp`) stores the element kind, `id`, the content `content` (`text`, `value`,
`checked`, `placeholder`…) and computed style. After changing the content from code, the node is marked
`ui::markDirty(node, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT)`: the engine re-lays out and repaints only what is needed.

**Templates.** A repeated block of the same look is better described as a list from data (the section below).
If a block is assembled in a more complex way (like a Markdown answer in `examples/chat/`), it can be inserted as a markup
fragment with `appendMarkup`, or a hidden sample can be copied with `ui::cloneTree` and inserted with `append`.

**Background threads.** Access to nodes can only be made from the UI thread. A network or computation thread
passes the result through `app.post(task)` — the task will be executed in the event loop, and the window will
wake up itself.

## Events

In addition to actions (`onClick`, `onChange`), any element has events, similar to those in a browser's DOM. The handler receives the node and the `ui::UiEvent` (`gran/ui/ui_event.hpp`) and can stop it or prevent the default engine action.

```xml
<Stack id="board" onKeyDownCapture="hotKeys">
  <Box id="pad" onPointerDown="padDown" onPointerMove="padMove" onWheel="padWheel" onContextMenu="padMenu"/>
</Stack>
```

```cpp
app.onEvent("padMove", [](gran::ui::Node& pad, gran::UiEvent& event) {
    // event.local — point in the pad's coordinate system (with its transform), event.position — window point
});
app.onEvent("padMenu", [&](gran::ui::Node&, gran::UiEvent& event) {
    event.preventDefault();                                   // no built-in "Copy / Paste" menu
    app.showPopupAt(*app.find("myMenu"), event.position);
});
```

**Propagation.** The event travels from the root to the target (capture phase, attributes with the `Capture` suffix), then at the target itself and back to the root (bubble phase). `event.stopPropagation()` stops the path (current node handlers are still called), `stopImmediatePropagation()` — immediately. `event.target` — the node to which the event is addressed, `event.current` — the node whose handler is called, `event.phase` — phase. The path is collected before delivery; if the handler removes nodes (`remove`, list model editing), delivery and the default action are stopped.

| Event | Attribute | Target | Cancellation (`preventDefault`) |
|---|---|---|---|
| `keyDown` / `keyUp` | `onKeyDown` / `onKeyUp` | focused node, if none — top layer or root | `keyDown`: Tab, field editing, Enter/Space, arrows and **characters** of this key |
| `pointerDown` / `pointerUp` | `onPointerDown` / `onPointerUp` | node under the mouse or capturing the pointer | press: focus, selection, slider, context menu; release: `onClick` and `doubleClick` |
| `pointerMove` | `onPointerMove` | the same | mouse selection, slider, sub-menu |
| `pointerEnter` / `pointerLeave` | `onPointerEnter` / `onPointerLeave` | each node entering the mouse chain or leaving it | — (do not bubble) |
| `doubleClick` | `onDoubleClick` | node under the mouse (after the second release) | — |
| `wheel` | `onWheel` | node under the mouse | scrolling |
| `focus` / `blur` | `onFocus` / `onBlur` | receiving / losing focus | — (do not bubble; `related` — second node) |
| `contextMenu` | `onContextMenu` | right-click, menu key, Shift+F10 | built-in text and field menu |
| custom | `on="name:action …"` | `Application::dispatchEvent` | by application agreement |

Non-cancelable events (`pointerEnter/Leave`, `focus/blur`, `dragEnter/Leave`, `dragEnd`) ignore `preventDefault`. Notifications `focus`, `blur`, `pointerEnter`, `pointerLeave` arrive after the built-in window event handling (order as in the browser: `pointerDown`, `blur`, `focus`, `pointerUp`, `onClick`). Keys — `event.key` (`platform::Key`: letters A–Z by physical key, digits, F1–F12, arrows…), name — `platform::keyName(key)` («A», «Enter», «ArrowLeft», «F5»), modifiers — `event.modifiers`.

**Handlers.** The name from the attribute is first searched among `onEvent`, then among `onAction` (the function receives only the node) and built-in commands: `onDoubleClick="gran:maximize"`, `onPointerDown="gran:capturePointer"` work without code. From the code — node listeners:

```cpp
gran::ListenerId id = app.addEventListener(*app.find("list"), "keyDown", handler, /*capture*/ true);
app.removeEventListener(*app.find("list"), id);   // listener goes away with the node
```

**Custom events.** `app.dispatchEvent(node, "itemChosen", "string")` follows the same path; markup catches it with the attribute `on="itemChosen:pickItem"`. The result is `false` if the handler called `preventDefault`. Thus, the element reports "upwards", not knowing who is listening to it (example: the "Reset Board" button in `samples/events.xml`).

**Pointer capture.** `app.capturePointer(node)` (or `gran:capturePointer` in markup) directs mouse events to the node, wherever the mouse is, until the button is released or `releasePointer(node)` — this is how custom sliders, size change handles, and drawing are written.

### Dragging nodes

```xml
<Stack items="todo" acceptDrop="task" onDrop="moveTask">
  <ItemTemplate>
    <Box style="card" draggable="true" dragFormat="task" dragData="{title}" onDoubleClick="flipTask">…</Box>
  </ItemTemplate>
</Stack>
```

A node with `draggable="true"` starts dragging when the mouse, with a button pressed, moves more than 4 pixels. Data — format `dragFormat` (default `node`) and string `dragData` (default `id`, in templates — substitutions `{field}`); `event.drag` in `dragStart` can change them, `preventDefault` — cancel dragging. Zone — the nearest node to the mouse with `acceptDrop`, in which the format exists (`*` — any); the source subtree is not a zone. The zone receives `dragEnter`, `dragOver` (cancellation — "not here"), `dragLeave` and `drop` (`event.related` — the dragged node), the source — `dragStart` and `dragEnd` (`event.dropped`). While dragging, the source is in the `<Dragging>` state, the agreeing zone — in `<DropTarget>`, the cursor is "move" or "not allowed"; Esc and window focus loss cancel it. `onDrop` in the same zone also accepts files from the Explorer (`onFileDrop`). A typical `drop` handler transfers the list model record:

```cpp
app.onEvent("moveTask", [&](gran::ui::Node& column, gran::UiEvent& event) {
    gran::ItemRef item = app.itemOf(*event.related);
    gran::data::Record record = item.model->at(item.index);
    item.model->remove(item.index);                    // the card node is deleted by binding
    app.model(column.content.items).append(record);
});
```

Complete example — `gran_gallery samples/events.xml` (handlers — `examples/gallery/gallery_board.cpp`).

## Commands and shortcuts

A `<Command>` (similar to `QAction` in Qt) is a single action for a button, menu item, and key combination.
Elements reference it via the `command` attribute; a disabled command disables all of them and stops responding to the keyboard.

```xml
<Resources>
  <Command id="saveDoc" text="Save" key="Mod+S" action="save"/>
  <Command id="refresh" text="Refresh" key="F5 Mod+R"/>
  <Command id="deleteNote" text="Delete" key="Del" scope="focus" within="notes" action="deleteNote"/>
  <Command id="quit" text="Exit" key="Mod+Q" action="gran:close"/>
</Resources>
…
<Button command="saveDoc"/>                          <!-- the "Save" label comes from the command -->
<Popup id="fileMenu">
  <MenuItem command="saveDoc"/>                      <!-- "Ctrl+S" on the right -->
  <MenuItem command="quit"/>
</Popup>
```

| Attribute | Value |
|---|---|
| `id` | command name (unique together with `id` of window elements) |
| `action` | action (`onAction`, `gran:…`); without it — action with the name `id` |
| `text` | label for buttons and menu items that do not have their own |
| `key` | combinations separated by spaces: modifiers `Mod` (= Ctrl), `Ctrl`, `Shift`, `Alt` through `+` and key — `S`, `F5`, `Enter`, `Del`, `Esc`, `Left`, `PgUp`, `Insert`… (case does not matter) |
| `scope` | `window` (by default) — in its own window; `focus` — while focus is inside the element `within`; `global` — in all program windows |
| `enabled` | `false` — command and its elements are disabled |

**Who receives the key.** First `keyDown` goes to the handlers (one cancelled with `preventDefault` never reaches the commands), then a command is looked up: the `focus` scope commands whose `within` contains the focus (the deepest one first), then the `window` and `global` ones of its window, then the `global` ones of the other windows. Commands do not intercept what the focused element does itself: in an input field — typing and editing (everything without Ctrl and Alt, except F1–F12; Ctrl+A/C/X/V/Z/Y, Ctrl+arrows, Home/End, Backspace/Delete; Ctrl+Alt — this is AltGr), on buttons, checkboxes, and sliders — arrows, space, and Enter, on selected text — Ctrl+A/C; Tab always transfers focus. When a menu or list is open, commands do not trigger, when a modal dialog is open — only `focus` commands inside it. Shortcuts work within the program, not system-wide. The triggered command consumes the key (its characters do not appear in the field).

**Execution.** The action receives the node: for a button and menu item — themselves, for a combination — the node in focus (without focus — the root of the window). Command labels are received only by elements without their own `text` and without children (an icon button remains without a label); the label of the first combination is visible on the right in the menu item and is available to screen readers (UIA AcceleratorKey property). The schema forbids `command` and `enabled` on one element: the command sets whether it is enabled.

```cpp
app.onAction("save", [&](gran::ui::Node& from) { /* … */ });
app.setCommandEnabled("saveDoc", false);   // button, menu item, and Ctrl+S are disabled
app.commandEnabled("saveDoc");             // false
app.runCommand("refresh");                 // execute from code; false — no such command or disabled
```

`setCommandEnabled` acts on all windows, survives a markup reload and applies to the windows opened later (can be called before `loadFile`). In the markup, the same operations — `gran:runCommand:id`, `gran:enableCommand:id`, `gran:disableCommand:id`. A button from another window can reference a command `scope="global"` from the main window.
Complete example — `gran_gallery samples/commands.xml` (actions — `examples/gallery/gallery_commands.cpp`).

## Undo and redo

**In fields.** Every `Input` and `TextArea` has its own edit history: Ctrl+Z undoes, Ctrl+Y and Ctrl+Shift+Z
redo; the same "Undo" and "Redo" items are in the field context menu (disabled when there are no steps).
Typing in a row is one step (as in browsers and Qt), so are Backspace in a row and Delete in a row. A new step
starts after a caret move (the mouse, arrows, Home/End, Ctrl+A), a typing pause longer than a second, pasting,
cutting, erasing a selection and a new line. Undo restores the text and the selection as they were before the edit
and calls `onChange` like an ordinary edit. A field history holds 100 steps; a text set from code (`gran_app_apply`,
a list record, a reload, assigning `content.text`) resets it. `undo="false"` on a field — the edits are not
recorded at all (a field whose text the code drives).

**In the program.** For your own actions (delete a note, move a shape) there are application undo stacks
(as `QUndoStack`): a step is pushed already done, undo and redo call its handlers.

```cpp
std::string removed = notes.take(index);              // the action is done
app.pushUndo([&, index, removed] { notes.insert(index, removed); },   // undo
             [&, index] { notes.take(index); });                     // redo
app.undo();                     // or <Command action="gran:undo"/>; false — nothing to undo
app.undoStack().setClean();     // the document is saved
app.onUndoChanged([&](const std::string& stack, const gran::UndoStack& state) {
    showTitle(state.isClean() ? name : "* " + name);  // an own function: "*" — unsaved changes
});
```

```xml
<Command id="undo" text="Undo" key="Mod+Z" action="gran:undo"/>
<Command id="redo" text="Redo" key="Mod+Y Mod+Shift+Z" action="gran:redo"/>
<Command id="undoLayer" key="Mod+Alt+Z" action="gran:undo:layers"/>   <!-- an own stack "layers" -->
```

Commands with the `gran:undo`, `gran:redo` action (`:name` — an own stack) are enabled and disabled by the stack
themselves — they need no `setCommandEnabled`. Stacks are created on the first access, `""` is the main one.

**The field and the program together.** `gran:undo` and `gran:redo` without a stack name work with the edited field
of the window first — as Ctrl+Z in it: "Edit → Undo" with the focus in a field undoes the typing, and when the field has
nothing to undo — a program step. The edited field is the focused field or the field the menu was opened from. Buttons
with `command` or `popup` (a toolbar, a menu button) do not take the focus from the field on a click, as in desktop
programs; Tab moves the focus to them as usual. The enabled state of these commands takes the field into account too.
Named stacks (`gran:undo:name`), as well as `app.undo()` and `gran_app_command("undo")` from code, work with the
program stack only.

| Operation | Meaning |
|---|---|
| `undoStack(name)` | The stack (`gran::UndoStack`): `push`, `undo`, `redo`, `canUndo`, `canRedo`, `isClean`, `setClean`, `clear`, `count`, `index`, `setLimit` (0 — no limit) |
| `pushUndo(undo, redo, name)` | Record a done action as a pair of handlers |
| `undo(name)`, `redo(name)` | The same as `gran:undo[:name]`, `gran:redo[:name]` |
| `onUndoChanged(handler)` | Called after every change of any stack (a step, undo, redo, `setClean`, `clear`) |
| `gran:clearUndo[:name]`, `gran:setUndoClean[:name]`, `gran:setUndoLimit:name:N` | The same operations from markup and the C ABI |

An own merging step is a `gran::UndoStep` descendant (`gran/undo/undo_stack.hpp`): `mergeId()` ≥ 0 and
`mergeWith(next)` absorbs the next step of the same kind until merging is interrupted (`breakMerge`, undo, redo,
`setClean`; steps do not merge on the clean position). The limit drops the oldest steps; if the clean position
went away with them, `isClean()` will not come back to it. A step handler does not call `undo`/`redo` of its own stack,
and the steps recorded during undo are ignored (the change is a consequence of undo, not a new edit).
The stacks survive a markup reload — steps find nodes by id (`app.find`) instead of keeping pointers.
Example — `gran_gallery samples/commands.xml` (notes: deleting and adding are undoable, "*" in the title).

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

- Substitutions `{field}` work in `text`, `placeholder`, `source`, `tooltip`, `accessibleName`, and in tag text (including
  in `Span`). `{{` and `}}` — the braces themselves; if the field does not exist in the record, it gives an empty string.
- Model editing changes only the affected copies: insertion and deletion — their own nodes, field change — only texts
  with substitutions. Focus, selection, and scrolling of other records are preserved.
- Record — strings (`gran::data::Record`: field name → text); the format of values is determined by the application.
- `<Data>` replaces the model's contents on loading; the model can be populated before loading the markup.
- Lists can be nested (template inside template), but the nested list is linked to the model by name — common
  for all records.
- The model changes only in the UI thread; the background thread passes data through `app.post`.
- **A long list** — `virtual="true"` on a `Stack` with `items` inside a `Scroll`: copies are built only for the records near
  the visible area (plus 8 on each side), two spacers replace the rest. A list of 100,000 records is built in a fraction
  of a millisecond and scrolls at ~1 ms per frame. The condition is records of about the same height: a record position
  goes through the average size, so with records of very different heights the scroll bar "drifts" a little.
  Copies outside the window do not exist: `app.find` and `itemOf` see only the records in the window, and the state inside
  a copy (selection, focus) is lost when its record leaves the window.
- `gran_snapshot` builds lists from `<Data>` without a window — the list frame is checked without the program
  (example — `samples/lists.xml`).

Markup fragment during operation — `app.appendMarkup(parent, "<Stack>…</Stack>")`: one root element,
validation by the same schema, styles `style="…"` and theme values `$name` — from the document. Error — `MarkupException`
with position inside the fragment; the tree does not change.

**Live document.** The markup is not thrown away after loading: its DOM stays in the window, and the nodes know their elements.
So an attribute can be changed "in the file" right at run time — `app.setMarkupAttribute(*app.find("sidebar"),
"width", "320")`: the edit is checked by the schema (an error is `MarkupException`, everything stays as it was), the node is rebuilt,
and the field texts, check marks, selected tabs and scrolling inside it are kept. `app.saveMarkup()` returns the window markup
with the edits, `appendMarkup` insertions and `remove` removals — it can be saved and loaded on the next
launch. Reloading the file (`reload`, `--watch`) became partial too: only the changed branches are rebuilt,
and the typed text and the focus are not lost. Details are in the [API](api.en.md#live-document).

## Binding expressions

The value of an attribute in curly braces is an XPath 1.0 expression: the engine evaluates it itself and recalculates it when what it depends on changes. The interface in the example `samples/bindings.xml` — search with a count — works without a single line of application code:

```xml
<Resources>
  <Data name="tasks">
    <Item title="Check email" done="true"/>
    <Item title="Check print" done="false"/>
  </Data>
  <Variable name="query"/>                                  <!-- variable $query, string -->
  <Variable name="details" type="boolean" value="true"/>
  <Variable name="panel" type="number" value="220"/>
</Resources>
...
<Input placeholder="Search" value="{$query}"/>               <!-- two-way: input writes to $query -->
<Text>Found {count(tasks/Item[contains(@title, $query)])} of {count(tasks/Item)}</Text>
<Stack items="tasks">
  <ItemTemplate>
    <Stack visible="{contains(@title, $query)}"
           background="{substring('#ffffff#eef2f6', 1 + 7 * ($index mod 2), 7)}">
      <CheckBox checked="{done}"/>                          <!-- two-way: writes the done field of the record -->
      <Text>{title}</Text>
    </Stack>
  </ItemTemplate>
</Stack>
<Button onClick="gran:toggleVariable:details">Details</Button>
<Stack id="side" width="{$panel}" visible="{$details}">
  <Slider min="160" max="300" value="{$panel}"/>
  <Text>Width: {$side.width} px</Text>
</Stack>
```

What the expression sees:

| Notation | Value |
|---|---|
| `$name` | application variable: `<Variable name type value>` in `<Resources>` (`type` — `string`, `number`, `boolean`) or `app.setVariable` |
| `$id.property`, `$self.…`, `$parent.…` | node property: `width`, `height` (after layout, logical pixels), `text`, `checked`, `value` (number for Slider/ProgressBar, selection for ComboBox/Tabs), `visible`, `enabled` |
| `model/Item` | list model records as XML: `count(files/Item)`, `files/Item[@done = 'true']` |
| `@field`, `{field}` | own record field in list template; `{title}` is the same as `{@title}` |
| `$index` | record number in the list (starting from 0; outside the list — 0) |

- **Where.** Entirely in `visible`, `enabled`, `checked`, `value`, and any style property (`width`, `opacity`, `background`, `color`…); mixed with text — in `text`, `placeholder`, `tooltip`, `source`, `accessibleName`, `dragData`, and tag text. In `<Style>`, states, keyframes, and `<Window>` attributes, there are no expressions (they are checked by the schema): the style is common for many nodes, but the expression belongs to one.
- **The language is the Beresta XPath 1.0**: division — `div`, remainder — `mod`, comparison — `=`, `!=`, `<` (in XML — `&lt;`), strings — `contains`, `concat`, `substring`, `string-length`, numbers — `round`, `floor`, `sum`. Spaces are needed around the minus sign: `$a.width - 10` (without a space `width-10` is part of the name). A syntax error is a `MarkupException` at load time with the line and column of the element.
- **Boolean attributes.** Text `"false"`, `"0"`, and empty — false; the record field is taken by text (`checked="{done}"` when `done="false"` — not checked), and a set of elements — by presence (`visible="{tasks/Item}"` — are there any records). Inside the expression, fields are strings: `@done = 'true'`. The `type="boolean"` variable inside the expression is a real boolean value: `not($details)`.
- **Two-way binding.** If the expression is simply `$name` or a record field, and the user changes the attribute (`value` for Input/TextArea/Slider/ComboBox/Tabs, `checked` for CheckBox/Switch/RadioButton), the user's edit is written back: to the variable or to the model record field. Everything else that depends on the same source is recalculated in the same frame.
- **Only what changed is recomputed.** Expression dependencies are collected at load; before the frame, the engine compares them with the previous ones (strings and version numbers, without evaluating XPath) and recalculates only those expressions where something has changed, and touches the node only if the result has changed. Expressions from sizes (`$side.width`) are recalculated after layout in the same frame; an expression of its own size may not converge — the number of passes is limited.
- **Converters.** `{expression ; name}` passes the value through an application function:

  ```cpp
  app.setConverter("bytes", {formatBytes, parseBytes});   // toView, toSource (empty — only one way)
  ```

  ```xml
  <Text>Size: {$size ; bytes}</Text>
  ```

- **Variables from code and markup.** `app.setVariable("query", "abc")` keeps the variable type (a new one is a string), `setVariableNumber` and `setVariableBoolean` set the type explicitly, `app.variable(name)` reads the value as text. Built-in commands `gran:setVariable:name:value` and `gran:toggleVariable:name` change the variable without code (button, menu item, `<Command action>`). Reloading the markup returns values from `<Variable>`, as `<Data>` returns records.
- Expressions are recalculated before each frame; code that needs the values right after editing models and variables calls `app.refreshBindings()`.
- Curly braces as text are written doubled: `{{` and `}}`.

## Tabs, radio buttons, switches

**Radio buttons** of one group are checked one at a time. Without `group` the group is the neighbor radio buttons
without a name; with `group` it is all the window radio buttons with this name, wherever they stand. The value is
`value` (without it — the label), `onChange` is called on the button that got checked. A **switch** `Switch` behaves
as a `CheckBox`.

```xml
<RadioButton text="Light" value="light" checked="true" onChange="theme"/>
<RadioButton text="Dark" value="dark" onChange="theme"/>
<Switch text="Sound" checked="true" onChange="sound"/>
<ProgressBar id="progress" value="65" width="320"/>
```

**Progress bar**: the track is the `ProgressBar` style background, the fill is `accentColor`; the height and the
rounding come from the style. The value from code — `app.setValue(*app.find("progress"), 80)`.

**Tabs**: `Tab` pages with a `text` label; the engine builds the header strip above them itself (the theme styles
`TabBar` and `TabHeader`, the selected header is in the `Checked` state). Only the selected page is visible; `value`
of `Tabs` is the selected page value (the page value is `value`, without it — `text`), without it the first one is selected.

```xml
<Tabs value="general" onChange="tabChanged">
  <Tab text="General" value="general"> … </Tab>
  <Tab text="Download" value="download"> … </Tab>
</Tabs>
```

The keyboard works as in browsers: Tab reaches only the checked radio button of a group and the selected tab, the arrows
move the choice inside the group (for tabs — left and right), space and Enter choose. A sample —
`gran_gallery samples/widgets.xml`.

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

**Menu without code.** A button with `popup="id"` opens its layer by itself: by a click, Enter, space, and the down arrow.
From the keyboard, focus immediately goes to the first item (the same applies to a menu opened by code in `onClick`).
Esc closes the menu and returns focus to the button.

**Submenu** — `Popup` inside `MenuItem`. It opens to the side of the item (to the right, and if there is not enough space — to the left), by hovering the mouse, the right arrow, or Enter; the left arrow and Esc close only it. The item with a submenu is marked with an arrow and does not execute the command itself; selecting an item in the submenu closes the entire chain.

```xml
<Button text="File ▾" popup="fileMenu"/>
<Popup id="fileMenu">
  <MenuItem text="Open…" onClick="open"/>
  <MenuItem text="Export">
    <Popup>
      <MenuItem text="PDF" onClick="exportPdf"/>
      <MenuItem text="PNG" onClick="exportPng"/>
    </Popup>
  </MenuItem>
</Popup>
```

**Dropdown list** `ComboBox` shows the label of the selected item (without selection — `placeholder`). Selecting another item changes the value and calls `onChange`; the value of the item is its `value`, and without it — the text.
Items are written directly in the tag or taken from the model — the same as [lists from data](#lists-from-data).
An open list is not narrower than the field, the focus in it is on the selected item, the selected item is marked with the `Checked` style.

```xml
<ComboBox id="theme" value="light" onChange="themeChanged">
  <MenuItem value="light" text="Light"/>
  <MenuItem value="dark" text="Dark"/>
</ComboBox>
<ComboBox id="size" placeholder="Choose…" items="sizes" onChange="sizeChanged">
  <ItemTemplate><MenuItem value="{key}" text="{name}"/></ItemTemplate>
</ComboBox>
```

```cpp
// In the themeChanged handler: the value is combo.content.option, the label is combo.content.text
app.selectOption(*app.find("size"), "m");   // select from code, onChange is not called
```

**Dialog** `Dialog` — modal layer: it appears in the center of the window, the window under it is dimmed (style property `scrim`, in the theme — semi-transparent dark), clicks and the wheel outside the dialog do not go anywhere, Tab and Shift+Tab move only through its elements. When opening, focus goes to the first element of the dialog, when closing, it returns where it was.
Esc closes the dialog if `closable="false"` is not set (then it can only be closed by code). Menus and lists inside the dialog work as usual and are drawn above it.

```xml
<Button text="Delete…" popup="confirm"/>
<Dialog id="confirm" closable="false">
  <Text fontWeight="700">Delete the file?</Text>
  <Stack orientation="horizontal" spacing="8" justify="end">
    <Button text="Cancel" onClick="cancel"/>
    <Button text="Delete" onClick="remove"/>
  </Stack>
</Dialog>
```

```cpp
app.showDialog(*app.find("confirm"));   // open from code
app.hideDialog(*app.find("confirm"));   // close (usually in a button handler)
```

Layers are drawn in the order of the tree: a nested layer (menu in a dialog, submenu) — above its own, so a dialog that opens from another dialog is conveniently declared inside it. Everything together — `samples/dialogs.xml` (`gran_gallery samples/dialogs.xml`).

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

## Accessibility

The window gives the element tree to screen readers (Narrator, NVDA, JAWS) and UI test tools through
Microsoft UI Automation. The element role comes from its kind, the name — in this order:

1. `accessibleName="…"` — an explicit name (as `aria-label`);
2. `labeledBy="id"` — the text of another element, for example a label next to a field (as `aria-labelledby`; the schema
   checks that such an `id` exists);
3. its own text; for `Input`, `TextArea` and `ComboBox` — `placeholder` (the field text is its value); for a button
   without text — the children texts;
4. `tooltip`.

```xml
<Text id="volumeLabel" text="Volume"/>
<Slider labeledBy="volumeLabel" min="0" max="100"/>
<Input accessibleName="Search contacts" placeholder="Find…"/>
```

Control patterns are what a client can do with the element by itself:

| Element | UI Automation pattern | What the client does |
|---|---|---|
| `Button`, `MenuItem`, any element with `onClick` | Invoke | press (as a click: onClick, a menu, choosing an item) |
| `CheckBox`, `Switch` | Toggle | toggle (the mark and onChange) |
| `RadioButton`, a tab header | SelectionItem | select (the group neighbors are cleared, onChange) |
| `Input`, `TextArea`, `ComboBox` | Value | read the text; for a field — replace it (as "select all and paste", with `maxLength` and onChange) |
| `Slider`, `ProgressBar` | RangeValue | read the number, minimum, maximum and step; for a slider — set it (with the step and onChange) |

A `readOnly` field, `ComboBox` and `ProgressBar` answer "read-only". Client actions run in the application loop
through the same functions as human input: a disabled element (`enabled="false"`, including through an ancestor)
and the elements under an open dialog are not pressed. A client can move the focus to an element that takes it
with Tab. Changes of the mark, the text and the number of the focused element are reported to screen readers as
events — they read the new value at once. A password is not disclosed: the value of a `password="true"` field is empty.

The window thread enters a COM single-threaded apartment (as for `WebView`): this way UI Automation calls the provider
on the window thread, through its message queue, and the tree is never read in parallel with changes. If the program
moved the thread into a multithreaded apartment before creating the window, the accessibility tree is not given out.

## Platform: windows, files, icons

The sample for the whole section is `samples/platform.xml` (`gran_gallery samples/platform.xml`).

**Multiple windows.** The first loaded window (`loadFile`) is the main one, closing it ends `run()`. Another window is `openWindow("window.xml")`: it has its own theme, styles, data, and tree, but actions (`onAction`), list models, timers, and tasks are shared. The method returns the root of the window's content; the window appears on the next pass of the loop. `closeWindow(node)` closes the window to which the node belongs (after the current event pass). `find(id)` searches in all windows, starting from the main one.

```xml
<Window title="Settings" width="380" height="200" onClose="settingsClosing">…</Window>
```

`onClose` — action on attempting to close the window (cross, Alt+F4). If it is defined, the window does not close on its own: the handler decides — `app.closeWindow(node)` or nothing (for example, first ask about unsaved changes). Without a registered handler, the window closes as usual.

**File dialogs.** Windows system dialogs, modal over the window of the node `owner` (by default — over the main one).
On Linux the dialog is shown by the system `zenity` (GTK) or `kdialog` (KDE, Fly) program: the engine runs it as a separate
process, no new libraries are needed; if neither exists — a `PLATFORM` exception with a hint.
While the dialog is open, the Linux window is not repainted. File dragging (`onDrop`) does not work on Linux yet. Empty result — the user declined.

```cpp
gran::FileDialogOptions options;
options.title = "Open an image";
options.filters = {{"Images", "*.png;*.jpg"}, {"All files", "*.*"}};
std::vector<std::string> files = app.openFileDialog(options, true, &node);   // several files
std::string target = app.saveFileDialog(options);                           // asks about replacing the file
std::string folder = app.folderDialog(options);
```

Parameter fields: `title`, `folder` (initial folder), `fileName` (suggested name), `filters`, `defaultExtension` (without a dot; added at save if the user did not write it). Paths are in UTF-8.

**File dragging.** The `onDrop="name"` attribute makes the element a drop zone. While files are over it, it has the `DropTarget` state (highlighting in the style), the cursor over the rest of the window shows "not allowed".

```xml
<Box onDrop="filesDropped" borderWidth="2" borderColor="#c3c8d2" transition="background 0.15s">
  <DropTarget borderColor="#2f6fed" background="#eaf1ff"/>
  <Text>Drop files here</Text>
</Box>
```

```cpp
app.onFileDrop("filesDropped", DropHandler{&app});   // void(ui::Node& zone, const std::vector<std::string>& paths)
```

**Icons.** `<Icon>` draws a vector figure in the color `color` (like text, with transitions and color animations). The default size is the size of `viewBox`; `width`/`height` stretch the icon, the raster is built under the final size in device pixels, so the icon is sharp at any scale and DPI.

```xml
<Icon data="M11 5h2v6h6v2h-6v6h-2v-6H5v-2h6z" color="$accent"/>      <!-- default viewBox 0 0 24 24 -->
<Icon data="M12 2a10 10 0 1 0 0 20a10 10 0 1 0 0-20zM12 6a6 6 0 1 0 0 12a6 6 0 1 0 0-12z" fillRule="evenodd"/>
<Icon source="icons/folder.svg" width="32" height="32"/>
```

- `data` — SVG path: all commands (`M L H V C S Q T A Z`, in both cases), incorrect path — error loading markup with position. `viewBox` and `fillRule` (`nonzero` or `evenodd`) — only together with `data`.
- `source` — `.svg` file: takes into account `viewBox` (or `width`/`height`) and filled `<path>`, `<rect>` (with rounding), `<circle>`, `<ellipse>`, `<polygon>`, `<polyline>` inside any `<g>`, `fill="none"` is skipped. Strokes, `transform` and CSS styles are not supported — for icons with strokes (Lucide, Feather) a filled version is needed.
- Corrupted or inaccessible file, like with `Image`, does not stop the window: the icon is simply not drawn.

**JPEG.** `Image` understands PNG and JPEG (format — by file content, not by extension): sequential and progressive JPEG, grayscale and colored, any color subsampling. CMYK, 12-bit and arithmetic coding are not supported.

## Errors and Message Language

The [C++ API reference](api.en.md) documents node lifetimes, threading, D3D11/D3D12 selection and markup
reloading. Run `gran_gallery samples/showcase.xml --watch` for automatic XML updates.
See [rendering tests](render_testing.en.md) for baselines and checks of both APIs.

All engine errors are exceptions `gran::GranException` (`gran/core/gran_exception.hpp`) with the code `ErrorCode`:

- `MarkupException` — markup or schema error: `getLine()`, `getColumn()`, path to the element in the text;
- `GpuException` — GPU error with the original `HRESULT` code.

Message texts are stored in the message catalog (Russian and English). The language is changed by calling
`gran::i18n::setLanguage("en")` (`gran/core/i18n.hpp`).

