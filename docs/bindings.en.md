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

## Core and language: who does what

A binding is a thin layer: each of its methods is one C ABI call, and all UI work happens in C++. For a program in
Python, C# or Java to be as fast as a C++ one, the language should tell the core **what** to do rather than do it itself.
A good rule: language code is reactions to actions, domain rules, and work with files, the network and the database;
everything about presentation is described in markup and done by the core.

| Task | How to do it in the core | What to avoid in the language |
|---|---|---|
| Change several properties | one `apply([(id, property, value), …])` — one call, one recalculation | a loop of separate calls for each property |
| Show a list of records | a model + `items="name"` and `<ItemTemplate>`; `model_insert` / `model_set` | creating elements with `appendMarkup` for each record |
| Load data from a file | `load_data` / `load_tree`: the core parses JSON and XML | parsing the file in the language and inserting records one by one |
| Sorting and search | `create_proxy`, `set_sort`, `set_filter` (XPath) — the source does not change | re-sorting and rebuilding the model on every keystroke in the search box |
| Visibility, enabled state, texts that depend on state | binding expressions and variables: one `set_variable`, the core recalculates every dependent attribute | walking nodes and calling `apply` after every mode change |
| Open a dialog or menu, close a window, undo | `gran:` commands right in the markup | a language handler that only calls `command` |
| Animations and smooth transitions | `transition` and keyframes in styles | a language timer changing a property every frame |
| Large lists and trees | `on_fetch` and lazy nodes — data arrives as the user scrolls and expands | inserting every record up front |
| Watching state | events: onChange, `on_selection_changed`, `on_undo_changed` | polling `read` on a timer |

**A handler must return quickly.** It runs on the UI thread: while it works, the window neither draws nor responds.
Anything longer than a few milliseconds (files, network, database, computations) belongs in `run_task`: the work runs on a
pool thread, `task.progress` moves the progress bar, and `done` receives the result back on the UI thread. Your own thread
is fine too, but then changes reach the UI only through `post`.

**Converters call the language from the core** on every binding evaluation. Keep them short and free of side effects;
simple formatting and arithmetic are better written as an XPath expression right in the markup.

**The model is for presentation, domain objects live in the language.** Field binding is one-way: onChange stores the new
value in the language object and, if needed, in the model with `model_set`. The model does not replace a database and
should not be the only store of business data.

**Language specifics.**
- Python: `ctypes` releases the GIL for the duration of a C call, so while `run` waits for events and draws a frame, other
  Python threads run. But a heavy pure-Python computation in `run_task` holds the GIL, and UI handlers wait for it. For such
  computations use a separate process (`multiprocessing`, `concurrent.futures.ProcessPoolExecutor`) or a library that
  releases the GIL (for example, NumPy); the result returns to the UI through `post`.
- C# and Java: C++ draws the frame while the UI thread is in native code, so a garbage collector pause does not stop
  animations — it only delays a handler's response. Do not allocate many temporary objects in frequent handlers (mouse
  movement, timers).
- Any language: handlers, converters and `post` tasks are held by the binding until `close` (or until removed or run),
  so closures should not keep large data alive longer than needed.

Example (Python): searching a list without walking records in the language.

```python
# Markup: <Input id="search" onChange="search"/> and a list with items="visible".
app.load_data("files", "files.json", select="files")         # JSON parsing is in the core
app.create_proxy("visible", "files", sort="name")             # sorting and filtering are in the core

def on_search(event):
    # The language only turns input into a condition; the core selects the rows.
    text = event.text.replace("'", "")
    app.set_filter("visible", f"contains(name, '{text}')")

app.on_action("search", on_search)
```

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
| `show_dock_panel(id, shown)` | `ShowDockPanel(id, shown)` | `showDockPanel(id, shown)` | `command("dockShow" / "dockHide", id)` |
| `reset_dock_layout(id)` | `ResetDockLayout(id)` | `resetDockLayout(id)` | `command("dockReset", id)` |
| `set_command_enabled(id, bool)` | `SetCommandEnabled(id, bool)` | `setCommandEnabled(id, bool)` | `command("enableCommand" / "disableCommand", id)` |

The handler receives a snapshot of `gran_input_event`: `type`, `target`, `current`, `related` (node ids),
`phase` (1 capture, 2 target, 3 bubble), `x`/`y` in the `current` system, `window_x`/`window_y`, `wheel_x`/`wheel_y`,
`button`, `clicks`, `modifiers` (the `Modifiers` bits: 1 `SHIFT`, 2 `MOD` — Ctrl on Windows and Linux, Cmd on macOS,
alias `CONTROL`; 4 `ALT` — Alt/Option; 8 `META` — Win/Super, the physical Control on macOS; in the C ABI — `GRAN_MODIFIER_*`),
`key` ("A", "Enter", "F5"), `detail`, `drag_format`,
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

### Live document

The markup stays live after loading (see the [API](api.en.md#live-document)): a node attribute can be read, written
with the schema check and a node rebuild (the state of the fields inside is kept), and the window markup can be saved to XML.
A schema error is a language exception (`RuntimeError`, `InvalidOperationException`, `IllegalStateException`), the markup stays as it was.

| Python | C# | Java | C ABI |
|---|---|---|---|
| `markup_attribute(target, name)` → `str` or `None` | `MarkupAttribute` → `string?` | `markupAttribute` → `String` or `null` | `gran_app_markup_read` (`*result == NULL` — none) |
| `set_markup_attribute(target, name, value)` (`None` removes) | `SetMarkupAttribute` | `setMarkupAttribute` | `gran_app_markup_write` (`value == NULL` removes) |
| `save_markup(window)` → XML | `SaveMarkup` | `saveMarkup` | `gran_app_markup_save` |

### Components

Component instances (see the [guide](guide.en.md#components)) are edited with the same `set_markup_attribute`,
the body nodes are found by the "instance.id" id. The generated library XSD is for XML editor hints:

| Python | C# | Java | C ABI |
|---|---|---|---|
| `component_schema(namespace, window=None)` → `str` (`""` — none) | `ComponentSchema(namespaceUri, window)` | `componentSchema(namespaceUri[, window])` | `gran_app_component_schema` |

### Binding expressions

The `$name` variables and the `{expression ; name}` converters of the markup expressions (see [guide](guide.en.md#binding-expressions)).
A bool / number / string value chooses the variable type; a string keeps the former type (a new variable is a string).
A converter is a "string → string" function; without `to_source` the binding is one-way. An exception in the converter
is handled as in any handler: the application closes and the error is thrown from `run`.

| Python | C# | Java | C ABI |
|---|---|---|---|
| `set_variable(name, value)` (`bool`, `int`/`float`, `str`) | `SetVariable(name, bool/double/string)` | `setVariable(name, boolean/double/String)` | `gran_app_variable_set_boolean/_number/_text` |
| `variable(name)` → `str` or `None` | `Variable` → `string?` | `variable` → `String` or `null` | `gran_app_variable_read` (`*result == NULL` — none) |
| `set_converter(name, to_view, to_source=None)` | `SetConverter(name, toView, toSource)` | `setConverter(name, toView, toSource)` | `gran_app_converter` + `gran_converted_set` |
| `refresh_bindings()` | `RefreshBindings()` | `refreshBindings()` | `gran_app_refresh_bindings` |

### Models: trees, views, selection

Tables, trees, sorting and filtering proxies, selection, and JSON/XML sources (see [guide](guide.en.md#models-tables-trees-sorting-selection)).
The names are shared with the markup. `model_count`/`model_read`/`model_set` work with any list (a proxy writes into the source).
Tree nodes have stable numbers, 0 is the top level. The `on_fetch` fetcher inserts records with `model_insert` and reports the end with `set_more(model, False)`. Handlers live until `close`.

| Python | C# | Java | C ABI |
|---|---|---|---|
| `model_move(model, start, count, to)` | `ModelMove` | `modelMove` | `gran_model_move` |
| `model_columns(model, [(name, title, "number")])` | `ModelColumns(model, Column[])` | `modelColumns(model, columns, types)` | `gran_model_columns` (`GRAN_COLUMN_*`) |
| `load_data(model, path, select, fetch, fields)` | `LoadData` | `loadData` | `gran_model_load` (`gran_source`) |
| `on_fetch(model, f(model))` / `set_more` / `fetch_more` | `OnFetch` / `SetMore` / `FetchMore` | `onFetch` / `setMore` / `fetchMore` | `gran_model_on_fetch` / `_set_more` / `_fetch_more` |
| `tree_insert(tree, parent, fields, index)` → number | `TreeInsert` → `ulong` | `treeInsert` → `long` | `gran_tree_insert` |
| `tree_remove` / `tree_move` / `tree_set` / `tree_read` | `TreeRemove` / `TreeMove` / `TreeSet` / `TreeRead` | `treeRemove` / `treeMove` / `treeSet` / `treeRead` | `gran_tree_remove/_move/_set/_read` |
| `tree_children(tree, node)` → list of numbers, `tree_parent` | `TreeChildren`, `TreeParent` | `treeChildren`, `treeParent` | `gran_tree_children` + `gran_tree_child`, `gran_tree_parent` |
| `tree_set_lazy(tree, node)`, `on_tree_fetch(tree, f(tree, node))` | `TreeSetLazy`, `OnTreeFetch` | `treeSetLazy`, `onTreeFetch` | `gran_tree_set_lazy`, `gran_tree_on_fetch` |
| `load_tree(tree, path, select, children)` | `LoadTree` | `loadTree` | `gran_tree_load` |
| `create_proxy(name, source, sort, filter)` | `CreateProxy` | `createProxy` | `gran_view_create` (`GRAN_VIEW_SORT_FILTER`) |
| `create_tree_list(name, tree, sort, filter, expanded)` | `CreateTreeList` | `createTreeList` | `gran_view_create` (`GRAN_VIEW_TREE_LIST`) |
| `create_selection(name, source, mode)` | `CreateSelection(name, source, SelectionMode)` | `createSelection(name, source, mode)` | `gran_view_create` (`GRAN_VIEW_SELECTION`) |
| `set_sort(view, keys)` / `set_filter(view, xpath)` | `SetSort` / `SetFilter` | `setSort` / `setFilter` | `gran_view_sort` / `gran_view_filter` |
| `source_index(view, row)` | `SourceIndex` | `sourceIndex` | `gran_view_source_index` |
| `tree_row(view, row)` → `{node, depth, expanded, has_children}`, `tree_expand(view, row, "toggle")` | `TreeRowOf` → `TreeRow`, `TreeExpand` | `treeRow` → `TreeRow`, `treeExpand` | `gran_tree_row_read`, `gran_tree_expand` |
| `selection_change(name, "range", index, last)` | `SelectionChange(name, SelectionCommand, index, last)` | `selectionChange(name, command, index, last)` | `gran_selection_change` (`GRAN_SELECTION_*`) |
| `selection(name)` → `(current or None, [numbers])` | `Selection` → `(Current, Selected)` | `selection` → `SelectionState` | `gran_selection_read` |
| `on_selection_changed(name, f(name))` | `OnSelectionChanged` | `onSelectionChanged` | `gran_selection_on_change` |

`command` supports `close`, `minimize`, `maximize`, `restore`, `reload`,
`focus`, `remove`, `showDialog`, `hideDialog`, `showPopup`, `hidePopup`, `navigate`,
`loadHtml`, `postMessage`, `appendMarkup`, `capturePointer`, `releasePointer`, `runCommand`, `enableCommand`,
`disableCommand` (for these three the target is a `<Command>` id, not a node one), `undo`, `redo`, `clearUndo`,
`setUndoClean`, `setUndoLimit` (the target is an undo stack name), `setVariable` (the target is a variable name, the argument
is the value), `toggleVariable` (flips a boolean variable). The markup also has `gran:expand`, `gran:collapse`, `gran:toggle`
(a flat tree row, at an element inside the template copy). The window commands of `command` belong to the main window.
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

### Windows and the program

Owner windows, modal windows, background tasks, the log, resources and settings (see the [guide](guide.en.md#windows-and-the-program)). A background task work
runs on a pool thread: a language exception inside it becomes a failure (`failed`) with a text, the result is a language object
the wrapper keeps itself until `done`. The owner is the id of any node of its window;
the close handler gets the result (`""` — the close button or closing with the owner) and is called once on the UI thread.

| Python | C# | Java | C ABI |
|---|---|---|---|
| `open_window(source, is_file=True, owner=None, modal=False, on_closed=None)` | `OpenWindow(source, isFile, owner, modal, onClosed)` | `openWindow(source, isFile, owner, modal, onClosed)` / `openWindow(path)` | `gran_app_open_window` (`gran_window_options`) |
| `close_window(target, result=None)` | `CloseWindow(target, result)` | `closeWindow(target, result)` | `gran_app_close_window` |
| `run_task(work, done, progress)` → id; `work(task)` returns the result, `task.cancelled()`, `task.progress(f, text)`; `done(status, result, error)`, `status` is `"done"`/`"cancelled"`/`"failed"` | `RunTask(work, done, progress)`; `BackgroundTask.Cancelled/Progress`; `BackgroundStatus` | `runTask(work, done, progress)`; `BackgroundTask.cancelled/progress`; `TASK_DONE/CANCELLED/FAILED` | `gran_app_run_task` (`gran_background`), `gran_task_cancelled/_progress/_fail` |
| `cancel_task(id)` → `bool` | `CancelTask` | `cancelTask` | `gran_app_cancel_task` |
| `open_log(app_name, directory=None, crash_reports=True)`, `log("info", category, text)`, `log_path()`, `previous_crash()` | `OpenLog`, `Log(LogLevel, …)`, `LogPath`, `PreviousCrash` | `openLog`, `log(LOG_INFO, …)`, `logPath`, `previousCrash` | `gran_log_open` / `_write` (`GRAN_LOG_*`) / `_path` / `_previous_crash` (process-wide) |
| `mount_resources(bytes or a path)` — a `gran_rcc --blob` block | `MountResources(byte[])` | `mountResources(byte[])` | `gran_resources_mount` (the bytes are copied) |
| `load_settings(app_name=None, path=None)`, `setting(name, default)` → `str`/`float`/`bool`/`list`, `set_setting(name, value)` (the type — by the value), `remove_setting`, `save_settings` | `LoadSettings`, `GetSetting` → `object?`, `SetSetting` (string/double/bool/list), `RemoveSetting`, `SaveSettings` | `loadSettings`, `getSetting` → `Object`, `setSetting`, `removeSetting`, `saveSettings` | `gran_settings_load/_get/_set/_list_item/_set_list/_remove/_save` (`GRAN_SETTING_*`) |

## C ABI

All strings are UTF-8 with a null terminator. Returned `char*` are only freed by
`gran_string_free`, even empty strings. Strings in input arrays must live until
the call returns. `gran_last_error` returns the calling thread's latest error text until its next error;
successful calls do not clear it. C++ exceptions are caught at the ABI boundary.

`gran_options.size` must be equal to `sizeof(gran_options)`;
`NULL` selects default settings. `gran_abi_version()` must return `GRAN_ABI_VERSION` (currently 1);
`gran_version_string()` and `gran_version_number()` report the version of the built library
(versioning rules are in [versioning.en.md](versioning.en.md)).
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
