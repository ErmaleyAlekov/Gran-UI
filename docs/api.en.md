# C++ API Gran UI

[Русский](api.md) · **English**

Headers are included from `include/`, for example `#include "gran/app/application.hpp"`.
Namespace — `gran`, standard — C++17. A ready-to-use example application — `examples/gallery`.
XML syntax and styles are described in the [guide](guide.en.md), library connection — in [building](building.en.md).
Python, C# and Java use a shared [C ABI and thin bindings](bindings.en.md).

## Application: window and event loop

`Application(AppOptions options = {})` creates a state; the window and GPU are created in `run()`.
`loadFile(const std::string&)` loads XML with schema validation; image paths and `<Font>` are relative to the file directory. `loadString(std::string_view)` loads a string with resources relative to the current folder.
`run()` shows the window after the first frame, performs input, tasks, timers, and animations, and returns the exit code.
In idle, the loop sleeps; without loaded markup, `run()` throws `GranException`.

`minimize()`, `maximize()`, `restore()`, and `close()` control the window on the UI thread, from a handler,
timer or `post` task. They do nothing before window creation. `close()` posts the usual close event
and ends the loop without deleting the tree inside a handler. `isMaximized() const` reads the OS state
and returns `false` before window creation. `maximize()` does nothing for `resizable="false"`.

`<Window titleBar="system">` keeps the standard frame and is the default. `titleBar="custom"` makes
the entire client area available to markup: `windowRole="drag|minimize|maximize|close"` supplies dragging
and window commands. The maximize role toggles maximize/restore. Command elements support Tab, Enter/Space
and UIA Invoke, then call their `onClick`, if present. `<Maximized>` applies throughout the tree, including
icons inside buttons. Commands work with both frame modes; drag/Snap geometry is used only in `custom`.
`titleBar` and `resizable` are creation parameters: changes through reload require a new application run.

The platform API exposes `platform::WindowDesc::titleBar` and the `platform::TitleBar` and `WindowRole`
enums in `gran/platform/window_frame.hpp`. `Window::setFrameHitTest(std::function<WindowRole(Vec2)>)`
reads roles on the UI thread in logical client pixels. `Application` connects its input router, respecting
transforms, clipping, disabled ancestors and modal layers. An empty callback disconnects the tree.

Fields of `AppOptions`:

- `debugLayer = false`: GPU debug layer and WebView tools.
- `vsync = true`: synchronization with the monitor.
- `exitAfter = 0`: exit after the specified number of seconds from the application clock; 0 disables the limit.
- `backend = rhi::BackendKind::AUTO`: D3D12 with a fallback to D3D11 on D3D12 creation error.
  `D3D11` and `D3D12` select the API explicitly: an error does not switch it to another.
- `forceWarp = false`: software WARP adapter instead of hardware, for reproducible tests.
- `watchMarkup = false`: monitoring of the XML file with a 250 ms interval during `run()`.

```cpp
#include "gran/app/application.hpp"

int main()
{
    gran::AppOptions options;
    options.backend = gran::rhi::BackendKind::D3D11;
    options.watchMarkup = true;
    gran::Application app(options);
    app.loadFile("samples/showcase.xml");
    return app.run();
}
```

## Markup Reloading

`reload()` reloads the last successfully loaded file; without a source file, it throws
`GranException(INVALID_ARGUMENT)`. `reloadIfChanged()` checks the file modification time and length;
returns `true` if a change is found and the new document is prepared, otherwise `false`.
A change in content with saved time and length is not detected: use `reload()`.

With `watchMarkup`, the check happens automatically, including when the window is idle.
Parsing, schema, or declared font errors retain the previous interface. The automatic mode writes
the error to stderr and `reloadError()`; a successful load clears the string. An erroneous file is rechecked
after the next change. Temporary absence of a file during atomic editor saving is skipped.
Manual load calls pass the exception to the calling code.

Inside `run()`, a successful load prepares the document, and replacement is performed after event processing,
tasks, and timers. Therefore, the handler can call `loadFile`, `loadString`, or `reload` without removing
its node in the middle of the call. Several loads before the loop boundary leave the last successful one.
Before `run()`, replacement is applied immediately. All these methods are called in the UI thread; from other threads — through `post`.

On replacement:

- only the main window is reloaded; the windows from `openWindow` and their nodes do not change;
- the OS window, GPU backend, action and WebView message handlers, tasks, and timers are preserved;
- the `<Font>` fonts of the new document are checked first and then added to the shared font library (other windows
  use it too, so the former fonts are not removed from it);
- the document, styles, animations, input, selection, popup layers, WebView browsers, and renderer caches are recreated;
- models obtained through `model()` are preserved; `<Data>` of the new document replaces data of identically named models;
- pointers and references to the previous `ui::Node`, including those captured by handlers, become invalid:
  repeat `find(id)` after replacement;
- `loadString` disables the source file; modified title, original size, and other window OS creation parameters
  are applied on a new launch. The current window size is saved, the background and content are updated.

Reloading observes the main XML. Changes to a separate PNG or font require `reload()` or
re-saving the XML. The DOM after parsing is not stored: a new verified document is built separately.
The guarantee of retaining the old interface applies to loading errors; memory/GPU exhaustion when applying
the prepared document may terminate the cycle with an exception.

For the gallery: `bin/gran_gallery samples/showcase.xml --watch --d3d11 --debug`.

## Nodes, Actions, and Data

`find(const std::string& id)` returns a non-owning `ui::Node*` or `nullptr`.
`onAction(name, ActionHandler)` registers or replaces a `void(ui::Node&)` handler by name from XML.
Handlers are executed in the UI thread. Changes from `setValue`, `setChecked`, `selectOption`, `selectTab`
do not call the user's `onChange` again.

- `setStyle(node, name)`: named style of the current document; unknown style — `RESOURCE` error.
- `append(parent, std::unique_ptr<ui::Node>)`: transfers ownership of the child to the tree, returns a reference to it.
- `appendMarkup(parent, std::string_view)`: one root XML fragment with schema validation, document styles
  and immediate connection of list bindings.
- `remove(node)`: removes the subtree, removes input, animation, and accessibility references. The root cannot be deleted.
- `focus(ui::Node*)`: focus from code, `nullptr` clears; before `run()`, the request is remembered.
- `model(name)`: reference to `data::ListModel`; unknown name creates an empty model.
- `itemOf(node)`: `ItemRef` of the nearest list entry; `model == nullptr` if the node is outside the binding.
- `setValue(node, float)`: Slider/ProgressBar value with boundary constraints.
- `setChecked(node, bool)`: CheckBox/Switch/RadioButton; selecting a radio button deselects group neighbors.
- `selectOption(comboBox, value)`: selection by value; unknown value deselects.
- `selectTab(tabs, value)`: page selection; unknown value retains the previous selection.
- `runCommand(id) -> bool`: run a `<Command id>` (the windows in order, the main one first) from its window root;
  `false` — there is no such command or it is disabled.
- `setCommandEnabled(id, bool)`: enable or disable the command in all the windows together with the `command="id"`
  controls (the Disabled state) and its shortcuts; the value is kept for a reload and new windows.
- `commandEnabled(id) -> bool`: whether the command is enabled; an unknown one is `false`.
- `undoStack(name = "") -> UndoStack&`: an undo stack (`gran/undo/undo_stack.hpp`, as `QUndoStack`), created
  on the first access: `push(unique_ptr<UndoStep>)` records an already done step (or absorbs it into the top one by
  `mergeId`/`mergeWith`), `undo`, `redo`, `canUndo`, `canRedo`, `isClean`/`setClean` ("the document is saved"), `clear`,
  `count`, `index`, `setLimit` (0 — no limit), `breakMerge`. The `gran:undo[:name]` / `gran:redo[:name]` commands (without a name — the focused field first)
  are enabled by the stack themselves.
- `pushUndo(undo, redo, name = "")`: record a done action as a pair of handlers (`undo::ActionStep`).
- `undo(name = "") -> bool`, `redo(name = "") -> bool`: undo / redo a step; `false` — nothing to do.
- `onUndoChanged(UndoHandler)`: a `(stack name, stack)` handler after every change of any stack.
- `Input`/`TextArea` edits go to the field's own history (`content.history`, Ctrl+Z/Ctrl+Y); assigning
  `content.text` from code invalidates it (reset on the next edit or undo).

When directly editing node fields, use `ui::markDirty(node, flags)` from `gran/ui/node.hpp`:
`DIRTY_STYLE` for cascade recalculation, `DIRTY_LAYOUT` for layout, `DIRTY_PAINT` for drawing.
Do not save a reference to a deleted node. Tree and model changes are performed in the UI thread.

## Events

`using EventHandler = std::function<void(ui::Node& node, ui::UiEvent& event)>` (`gran/ui/ui_event.hpp`),
`node` — the node whose handler is called (`event.current`). Everything — only in the UI thread.

- `onEvent(name, EventHandler)`: event handler from the event attributes (`onKeyDown`, `onPointerMove`,
  `onDropCapture`, `on="custom:name"`…); replaces the previous one. Without it, the name is searched among `onAction` and `gran:…`.
- `addEventListener(node, type, EventHandler, capture = false) -> ListenerId`: listener for the node; `type` —
  built-in name (`pointerDown`, `keyUp`, `dragOver`…) or custom. The listener is stored in the node (`ui::Node::listeners`),
  `cloneTree` does not copy it. `removeEventListener(node, id)` — remove (can be from the listener itself).
- `dispatchEvent(target, type, detail = "", bubbles = true) -> bool`: custom event along the capture → target →
  bubble route; `false` — `preventDefault` was called. Works even before `run()` (without the OS window).
- `capturePointer(node)` / `releasePointer(node)`: capture the mouse until the button is released; before `run()` — nothing.

`ui::UiEvent`: `type` (`ui::EventType`), `name`, `target`, `current`, `related`, `phase` (1 capture, 2 target,
3 bubble), `position` (window, logical pixels), `local` (current system: `ui::windowToNode` minus the top-left corner of the frame), `wheel`, `button`, `key`, `modifiers`, `clicks`, `detail`, `drag` (`format`, `data`), `dropped`,
`bubbles`, `cancelable`, `defaultPrevented`, `handled`; methods `stopPropagation()`, `stopImmediatePropagation()`,
`preventDefault()`. `ui::makeEvent(type, target, name)` builds an event with flags as in DOM, `ui::eventName` /
`ui::eventType` translate type to name and back, `platform::keyName(Key)` — key name.

Routing — `input::dispatchEvent(event, sink)` (`gran/input/input_router.hpp`); in your
`input::ActionSink` its handlers from the markup go to the virtual `event(name, node, event)` (by default —
`action(name, node)`), and `treeVersion()` reports on node removals (delivery stops).
The router passes the `<Command>` commands (`gran/ui/command.hpp`: `ui::Command`, `ui::Shortcut`, `parseShortcut`,
`shortcutText`) to `shortcut(CommandQuery)` (a shortcut, `true` — it ran) and `command(id, node)` (a button or
a menu item with `command`); the search by a combination with scopes is `input::matchCommand(sources, query)`.
The loader puts the commands into `UiDocument::commands` and binds the controls of its own document to them at once.

## Tasks, Timers, and Layers

`post(Task)` is safe from other threads: the task is copied to the queue and wakes the window; executed
in the UI thread, after which a frame is needed. Tasks not executed before exit are deleted with the application.
Captured objects must live until task execution.

`setTimeout(double seconds, Task)` returns `TimerId` of a one-time timer.
`setInterval(double seconds, Task)` creates a periodic timer (minimum 1 ms).
`clearTimer(TimerId)` cancels the timer, including self-cancellation from the handler; unknown id is safe.
Timers do not create threads and trigger no earlier than the deadline on the next loop pass;
missed periods are not caught up. Time is counted from the start, including before `run()`.

`showPopup(popup, anchor)`, `showPopupAt(popup, Vec2)` and `hidePopup(popup)` manage popup layers.
`showDialog(dialog)` and `hideDialog(dialog)` manage modal layers inside the window.
These methods are intended for a running `run()`; the dialog returns focus after closing.

## WebView

`onWebMessage(name, WebMessageHandler)` sets a handler `void(ui::Node&, const std::string&)`.
`postWebMessage(node, text)` passes a string to the page; messages wait in the queue before the browser is ready.
`navigate(node, url)` changes the address, `loadHtml(node, html)` loads HTML and resets `source`.
Passing a regular node instead of WebView is an error. Capabilities and limitations of the system browser are described
in the [guide](guide.en.md#embedded-browser-webview).

## Low-Level Loading and Rendering

`markup::loadMarkupFile(path, LoadOptions)` and `loadMarkupString(text, LoadOptions)` return owning
`UiDocument`. `LoadOptions::baseDirectory` sets the resource base, `maxNodes = 100000` limits the size.
`markup::buildFragment(text, document)` returns a separate tree with styles of this document.
Pointers to style rules require the original document to live longer than the using tree.

`ui::resolveStyles` calculates the cascade; `layout::layoutTree(root, Size, ContentMeasurer&)` places the tree.
`render::Renderer(RenderBackend&, FontLibrary&, imageDirectory)` provides `measurer()` for layout
and `render(root, windowStyle, scale, caret, selection)`. The renderer must be destroyed before the backend:
its destructor returns textures. The backend must be destroyed before the OS window.

`rhi::createPlatformBackend(BackendOptions)` selects the API; `createDx11Backend` and `createDx12Backend`
create specific backends (Windows 10/11). `BackendOptions` contains `nativeWindow` (HWND or `nullptr`),
size in physical pixels, `vsync`, `debugLayer`, `backend`, and `forceWarp`.
Without a window, `SnapshotSource::readPixels(rgba, width, height)` — dense RGBA8 top-down,
waiting for GPU completion. `resize(0, 0)` is safely skipped for minimized windows.

`RenderBackend` accepts `FrameData`, creates/updates/frees texture slots, and returns
`takeDiagnostics()` with cleared accumulated layer messages. `render()` returns `PRESENTED`,
`OCCLUDED` or `DEVICE_LOST`. After device loss, GPU caches need to be restored from CPU data;
`Application` does this automatically. All backend calls are performed by one render thread.

D3D11 requires FL11_0/SM5, uses built-in DXBC and fixed SRV registers.
Batches are split on image change; text, gradients, shadows, masks, transparency, and backdrops use
the same HLSL as D3D12. Up to 4095 user textures are available simultaneously (one slot is reserved).
D3D11 manages the lifetime of sent resources and versions of dynamic constants through runtime.
Texture sizes are limited to 16384, instance/clip buffers — 128 MiB each.
A regular program does not need fxc/dxc; shader update: `make shaders-dx11` / `make shaders`.

## Errors and Snapshot Verification

`GranException::getCode()`, `getMessage()`, `what()` provide category and translated message.
`MarkupException` adds `getLine()`/`getColumn()`, `GpuException` — `getNativeCode()`.
`i18n::setLanguage("ru"|"en")` selects the message catalog.

`render::comparePixels(actual, expected, tolerance)` compares `image::Image` by RGBA8.
`PixelComparison` contains `sameSize`, `differentPixels` (at least one channel exceeds tolerance)
and `maxDelta` (actual maximum difference, including tolerance).
Incorrect array length or size — `sameSize == false`.

[Rendering Tests](render_testing.en.md) describe baselines, tolerances, and Make/CMake commands.
