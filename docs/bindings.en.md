# Language Bindings

[Русский](bindings.md) · **English**

The general library `gran_c` provides C ABI version 1, described in
[`gran.h`](../include/gran/capi/gran.h). Python, C#, and Java call this interface.
XML parsing, styles, templates, list bindings, layout, input, animations, and rendering
are performed in C++. The application code in the chosen language contains handlers and data.

## Building

Windows x64: `make bindings` creates `bin/gran_c.dll` and uses the already built
Beresta library. CMake builds the `gran_c` target by default; disable it with:
`-DGRAN_BUILD_BINDINGS=OFF`. The library uses the same compiler and system dependencies as the engine. With WebView2 enabled, the DLL loader must be located next to the process executable, as in C++ applications.

Linux x86_64: the same target builds `libgran_c.so` (Vulkan + X11). Python finds the library by
`GRAN_UI_LIBRARY`, C# by `LD_LIBRARY_PATH` (the name `gran_c`), Java gets the absolute path to the `.so`.
macOS is not supported yet.

## General Model

The application is created, loads XML, gets handlers, and enters `run`.
Window closing (`command("close")`) and application disposal (`close`/`Dispose`)
are separate operations. Disposal is possible after exiting `run`.

All operations except `post` are called in the UI thread that created the application. The background thread passes changes through `post`; the user guarantees that the descriptor still exists. It is not possible to free the application and send tasks simultaneously.

Nodes are addressed by string `id`: the core re-finds the node before each operation.
After deletion or reload, the remaining `id` in the code is safe: the call will indicate that the element is missing. There are no external `Node*` or references to tree memory in the ABI.
Addressable ids must be unique in the materialized tree; for repeated template
elements, update the model using the event's `model` and `index`.

`read` supports `text`, `value`, `checked`, `visible`, `enabled`, `option`,
`style`, `kind`. `apply` accepts an array of triples `(id, property, value)`;
all listed properties are written except `kind`. Numbers and boolean values are passed as strings (`"42"`, `"true"`). `value` refers to Slider and ProgressBar, `checked` to CheckBox/Switch/RadioButton, `option` to Tabs/ComboBox.
For Input/TextArea, the text is in `text`. These changes do not trigger onChange.

The batch runs sequentially in a single core call, without intermediate frames.
This is not a transaction: earlier operations remain applied on error. The core then recalculates the marked branches.

`model_insert`/`model_set` pass record fields immediately as a dictionary. Model names
match `items="name"` in XML. `<ItemTemplate>` and `{field}` substitutions are processed by the core; wrappers do not clone elements and do not interpret the markup.
`model_read`, `model_count`, `model_remove`, `model_clear` work with the same models. Field binding remains one-way: the onChange handler can explicitly save the new value to the model.

## Events and Commands

Named handlers `on_action` correspond to onClick/onChange/onSubmit,
`on_message` — WebView messages. The event contains `target`, `model`, `index`,
`text`, `option`, `message`, `value`, `checked`. For a template element, `model` and
`index` point to the record, even if the child element has no id. Outside the list,
the native `index` is equal to `SIZE_MAX`. Strings are copied to managed memory before
passing to the user handler.

Exceptions in handlers remain in the application language: the wrapper remembers the first,
requests window closure, and rethrows the exception after `run`.
The `onAction`/`onMessage` handlers are held until the application is freed. `post` tasks and timers
go through one shared trampoline: a one-shot task is released after its call, a timer after firing
(`repeat=false`) or `clear_timer`, so frequent `post` calls from a worker thread do not accumulate memory.
The number of waiting tasks is `pending_tasks()` / `PendingTasks` / `pendingTasks()`.
Timers and background tasks are executed in the UI thread.

### UI events

Event attributes (`onPointerDown`, `onKeyDownCapture`, `onDrop`, `on="custom:name"`, see «Events» in
[guide](guide.en.md#events)) are linked to the language by name as well:

| Python | C# | Java | C ABI |
|---|---|---|---|
| `on_event(name, f)` | `OnEvent(name, f)` | `onEvent(name, f)` | `gran_app_on_event` |
| `listen(id, type, f, capture)` → number | `Listen(...)` | `listen(...)` | `gran_app_listen` |
| `unlisten(id, number)` | `Unlisten` | `unlisten` | `gran_app_unlisten` |
| `dispatch(id, type, detail)` → `bool` | `Dispatch` | `dispatch` | `gran_app_dispatch` |
| `capture_pointer(id)` / `release_pointer(id)` | `CapturePointer` / `ReleasePointer` | `capturePointer` / `releasePointer` | `command("capturePointer", id)` |
| `run_command(id)` | `RunCommand(id)` | `runCommand(id)` | `command("runCommand", id)` |
| `set_command_enabled(id, bool)` | `SetCommandEnabled(id, bool)` | `setCommandEnabled(id, bool)` | `command("enableCommand" / "disableCommand", id)` |

The handler receives a snapshot of `gran_input_event`: `type`, `target`, `current`, `related` (node ids),
`phase` (1 capture, 2 target, 3 bubble), `x`/`y` in the `current` system, `window_x`/`window_y`, `wheel_x`/`wheel_y`,
`button`, `clicks`, `modifiers` (1 Shift, 2 Ctrl, 4 Alt), `key` ("A", "Enter", "F5"), `detail`, `drag_format`,
`drag_data`, `dropped`, `model`/`index` of the `current` node and `related_model`/`related_index` of the `related`
(node being dragged — immediately its record). Snapshot methods `prevent_default()` / `PreventDefault()` /
`preventDefault()`, `stop_propagation()` and `stop_immediate_propagation()` set flags that are sent to the core
after the handler returns (in C ABI — return value `GRAN_EVENT_*`). The `listen` listener lives in the node and is removed with it; `dispatch` works even before `run`.

### Undo and redo

The application undo stacks (see "Undo and redo" in the [guide](guide.en.md#undo-and-redo)); the stack name
`None`/`null` is the main one. A step is pushed already done: the wrapper keeps a pair of functions and lets it go when
the stack dropped the step (a new step over undone ones, the limit, `clear_undo`, closing the application). `Input`
and `TextArea` fields keep their own histories (Ctrl+Z in a field) and do not write to these stacks.

| Python | C# | Java | C ABI |
|---|---|---|---|
| `push_undo(undo, redo, stack)` | `PushUndo(undo, redo, stack)` | `pushUndo(undo, redo, stack)` | `gran_app_undo_push` |
| `undo(stack)` / `redo(stack)` | `Undo` / `Redo` | `undo` / `redo` | `command("undo" / "redo", stack)` |
| `undo_state(stack)` → `UndoState(can_undo, can_redo, clean)` | `GetUndoState` → `UndoState` | `undoState` → `UndoState` | `gran_app_undo_state` (`GRAN_UNDO_*` flags) |
| `set_undo_clean(stack)` | `SetUndoClean` | `setUndoClean` | `command("setUndoClean", stack)` |
| `clear_undo(stack)` | `ClearUndo` | `clearUndo` | `command("clearUndo", stack)` |
| `set_undo_limit(n, stack)` | `SetUndoLimit` | `setUndoLimit` | `command("setUndoLimit", stack, "n")` |
| `on_undo_changed(f(stack, state))` | `OnUndoChanged` | `onUndoChanged` | `gran_app_on_undo` |

The `<Command action="gran:undo">` / `gran:redo` commands (`gran:undo:name` — an own stack) are enabled by the stack
themselves; `on_undo_changed` is handy for "*" in the title (`not state.clean`).

`command` supports `close`, `minimize`, `maximize`, `restore`, `reload`,
`focus`, `remove`, `showDialog`, `hideDialog`, `showPopup`, `hidePopup`, `navigate`,
`loadHtml`, `postMessage`, `appendMarkup`, `capturePointer`, `releasePointer`, `runCommand`, `enableCommand`,
`disableCommand` (for these three the target is a `<Command>` id, not a node one), `undo`, `redo`, `clearUndo`,
`setUndoClean`, `setUndoLimit` (the target is an undo stack name). The window commands of `command` belong to the main window.
For `showPopup`, the argument is the id of the anchor;
for `appendMarkup`, it is an XML fragment that the core checks against the common XSD.

In the markup, the same commands are available without a language handler:

```xml
<Button text="Open" onClick="gran:showDialog:settings"/>
<Button text="Close" onClick="gran:hideDialog:settings"/>
<Button text="Exit" onClick="gran:close"/>
```

Format: `gran:command[:id[:argument]]`. The `gran:` prefix is reserved by the core.
For `showPopup` without an argument, the anchor is the clicked button itself (it needs no id).
`close`, `minimize`, `maximize`, `restore` without an id act on the window where the button is clicked (including a window
from `openWindow`). An error in a markup command (an unknown id, invalid XML on `reload`) does not terminate
the application: the catalog error text goes to stderr.
Normal action names invoke registered business logic.

## C ABI

All strings are UTF-8 with a null terminator. Returned `char*` are only freed by
`gran_string_free`, even empty strings. Strings in input arrays must live until
the call returns. `gran_last_error` returns the calling thread's latest error text until its next error;
successful calls do not clear it. C++ exceptions are caught at the ABI boundary.

`gran_options.size` must be equal to `sizeof(gran_options)`;
`NULL` selects default settings. `gran_abi_version()` must return 1.
Codes: `GRAN_OK`, `GRAN_ERROR`, `GRAN_INVALID_ARGUMENT`, `GRAN_WRONG_THREAD`,
`GRAN_BUSY`. User C callbacks must not throw exceptions.
Their function and user data must remain valid until the handler is replaced/removed or the application is destroyed;
for a sent task — until execution or application destruction.
The DLL remains loaded until the process ends: registered Win32 classes and COM services can survive the individual application. Java caches library loading in the global arena, and the arena callbacks are freed after application destruction; recreating Application in the same JVM is supported.

Python uses standard `ctypes`, C# — P/Invoke (.NET 9), Java — standard Foreign Function & Memory API (JDK 22+, checked on JDK 25; 64-bit process).
JNA, JNI bridge, Python extension, or third-party packages are not needed.

## Running Examples and Checks

Commands are executed from the project root after `make bindings`.

Python 3: `python bindings/examples/python/counter.py`.
You can set `GRAN_UI_LIBRARY` or the `library` argument when creating Application.
Checks: `python bindings/tests/test_python.py -v`.

C#: add `bin` to PATH on Windows, then
`dotnet run --project bindings/examples/csharp/Counter.csproj`.
Checks: `dotnet run --project bindings/tests/csharp/Smoke.csproj`.
In your own project, add a ProjectReference to `bindings/csharp/GranUI.csproj`
and place the native DLL in the program directory or in the OS load path.

Java: `javac -d bin/java bindings/java/src/granui/Application.java bindings/examples/java/Counter.java`,
then `java --enable-native-access=ALL-UNNAMED -cp bin/java Counter <absolute-path-to-gran_c.dll>`.
Checks: compile instead of Counter the file `bindings/tests/java/Smoke.java`
and run the Smoke class with the same DLL argument. For a named module instead
of ALL-UNNAMED, specify the name of your module.

All examples use one `bindings/examples/counter.xml`. The `add`
handler only adds a model record; dialog buttons work through built-in commands.
Native C API checks are included in `make test`. Live language tests use
D3D11/WARP, real keyboard events, garbage collection, post, timers, and callback exceptions;
C# and Java additionally check several Applications in one process.
