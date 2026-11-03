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

The reload is partial. If the `<Window>` parameters and `<Resources>` (styles, theme, data, commands) did not change,
the new DOM is compared with the live one: equal branches stay as they were (the same `ui::Node` objects, listeners, state,
list record copies), changed ones are rebuilt in place — the changed node itself stays the same object,
its descendants are built anew. Adding or removing a child rebuilds the parent. Models are not touched.
Otherwise the tree is replaced as a whole (below). In both cases the user state — the `Input`/`TextArea` text
with the caret and history, check marks, Slider/ProgressBar values, the ComboBox and Tabs choice, `Scroll` scrolling and
the focus — moves to the new nodes with the same `id`, and for nodes without an `id` — by place relative to the nearest ancestor with an `id`.

On a full replacement:

- only the main window is reloaded; the windows from `openWindow` and their nodes do not change;
- the OS window, GPU backend, action and WebView message handlers, tasks, and timers are preserved;
- the `<Font>` fonts of the new document are checked first and then added to the shared font library (other windows
  use it too, so the former fonts are not removed from it);
- the document, styles, animations, input, selection, popup layers, WebView browsers, and renderer caches are recreated;
- models obtained through `model()` are preserved; `<Data>` of the new document replaces data of identically named models;
- pointers and references to the previous `ui::Node`, including those captured by handlers, become invalid:
  repeat `find(id)` after replacement (on a partial one — for the nodes of the rebuilt branches);
- `loadString` disables the source file; modified title, original size, and other window OS creation parameters
  are applied on a new launch. The current window size is saved, the background and content are updated.

Reloading observes the main XML. Changes to a separate PNG or font require `reload()` or
re-saving the XML. The new document is first built and checked as a whole, and only then compared
with the live one and applied.
The guarantee of retaining the old interface applies to loading errors; memory/GPU exhaustion when applying
the prepared document may terminate the cycle with an exception.

For the gallery: `bin/gran_gallery samples/showcase.xml --watch --d3d11 --debug`.

## Live Document

The Beresta DOM stays in the window document after loading (`markup::UiDocument::dom`), and every node built
from the markup knows its element (`ui::Node::domElement`; nodes from code, list record copies, the Tabs header
strip and the ComboBox layer have `nullptr`). The markup becomes a source of truth that can be changed
at run time and saved:

- `markupAttribute(node, name) -> std::optional<std::string>`: the node element attribute — from the file or written
  by code; `nullopt` — no attribute or the node is not from the markup.
- `setMarkupAttribute(node, name, value)`, `removeMarkupAttribute(node, name)`: write or remove the attribute
  in the DOM and rebuild the node as if it were written so in the file. The whole document is checked by the schema; a schema
  or value error is `MarkupException`, the DOM and the interface stay as they were. The node itself stays the same object (references
  to it, `addEventListener` listeners, hover and focus are kept), its descendants are built anew —
  look the references to them up again with `find`. The descendants state moves as on a reload; writing `text`,
  `value` or `checked` changes the state of the node itself. Children added at run time (`append` from code)
  stay in place. A node not from the markup is `GranException(INVALID_ARGUMENT)`.
- `appendMarkup(parent, …)` under a parent from the markup inserts the element into the DOM too; `remove(node)` removes its element.
- `saveMarkup(window = nullptr) -> std::string`, `saveMarkupFile(path, window = nullptr)`: the live markup of the window
  of the node `window` (the main one by default) as XML: with the edits, insertions and removals, indented by two spaces,
  comments are kept. Nodes from code and the state (typed text, model data) are not in the markup.

```cpp
gran::ui::Node& panel = *app.find("sidebar");
app.setMarkupAttribute(panel, "width", "320");      // schema check, the panel is rebuilt
app.saveMarkupFile("layout.xml");                   // the markup with the new width
```

The lower level is `gran/markup/live_document.hpp`: `editMarkupAttribute`, `rebuildFromElement`, `serializeMarkup`,
`detachMarkup`, `planPatch`. The application changes the window tree nodes (it needs the bindings, the input and the animations), so
use the `Application` methods from application code.

## Components

Components are declared in `<Library namespace>` library files and included with `<xi:include>` in `<Resources>`
(the syntax is in the [guide](guide.en.md#components)). For code an instance is an ordinary node: the expansion root
with the instance id; the body nodes are `find("instance id.id")`.

- `setMarkupAttribute(*app.find("card"), "title", "…")` / `removeMarkupAttribute` on an instance edit a parameter
  or a host attribute with a check by the schema "gran-ui.xsd + the library schemas"; the expansion is built anew.
  The body nodes are read-only: `GranException(INVALID_ARGUMENT)` with the MARKUP_COMPONENT_INTERNAL text.
- `saveMarkup` saves the instances and `<xi:include>`, not the expansion.
- `componentSchema(namespaceUri, window = nullptr) -> std::string` is the generated XSD of a library
  (empty — no such library). It is put next to `gran-ui.xsd` for XML editor hints.
- `Options::watchMarkup` watches the library files too: editing them reloads the window (partially if the prologue
  and the libraries after the edit match the former ones; otherwise fully, carrying the state over).

The level below is `gran/markup/components.hpp`: `componentSchemas(document)` (all the document schemas: the namespace,
the file, the text) and `libraryFiles(document)`.

## Nodes, Actions, and Data

`find(const std::string& id)` returns a non-owning `ui::Node*` or `nullptr`.
`onAction(name, ActionHandler)` registers or replaces a `void(ui::Node&)` handler by name from XML.
Handlers are executed in the UI thread. Changes from `setValue`, `setChecked`, `selectOption`, `selectTab`
do not call the user's `onChange` again.

- `setStyle(node, name)`: named style of the current document; unknown style — `RESOURCE` error.
- `append(parent, std::unique_ptr<ui::Node>)`: transfers ownership of the child to the tree, returns a reference to it.
- `appendMarkup(parent, std::string_view)`: one root XML fragment with schema validation, document styles
  and immediate connection of list bindings; under a parent from the markup the fragment also goes into the live DOM.
- `remove(node)`: removes the subtree (and its element from the live DOM), removes input, animation, and accessibility references.
  The root cannot be deleted.
- `focus(ui::Node*)`: focus from code, `nullptr` clears; before `run()`, the request is remembered.
- `model(name)`: reference to `data::ListModel` (a storage: `insert`, `insertAll`, `append`, `remove(index[, count])`,
  `set`, `setRecord`, `move(from, count, to)`, `clear`, `setColumns`, `setFetcher`, `fetchMore`); unknown name
  creates an empty model.
- `list(name)`: any list — storage, proxy, or flat tree (`data::ListSource`: `size`, `at`, `set`, `columns`,
  `canFetchMore`, `fetchMore`, subscription `ListObserver` — `inserted`, `removed`, `changed`, `changedRange`, `moved`,
  `reordered`); none — creates an empty storage.
- `tree(name)`: `data::TreeModel` (nodes — stable `NodeId`: `root`, `childCount`, `child`, `parent`, `row`, `record`,
  `insert`, `append`, `remove`, `move`, `set`, `clearChildren`, `appendItems` (a batch), `setLazy` + `setFetcher(TreeFetcher)`, subscription
  `TreeObserver`); moving a node into its own subtree — `GranException` (`DATA_TREE_CYCLE`).
- `createProxy(name, source)` → `data::SortFilterProxy` (`setSort(parseSortKeys("…"))`, `setFilter(RecordFilter)`,
  `invalidateFilter`, `sourceIndex`, `proxyIndex`); `createTreeList(name, tree)` → `data::TreeList` (`expand`,
  `collapse`, `toggle`, `expandToDepth`, `depth`, `expanded`, `hasChildren`, `node`, `rowOf`, `setSort`,
  `setFilter(NodeFilter)`); `createSelection(name, source, mode)` → `data::SelectionModel` (`select`, `deselect`,
  `toggle`, `selectRange`, `selectAll`, `clear`, `setCurrent`, `activate(index, ctrl, shift)` — like a click,
  `current`, `anchor`, `isSelected`, `selected`, `count`, `version`, subscription `SelectionObserver`).
  `proxy(name)`, `treeList(name)`, `selection(name)` — existing (none — `GranException`, `DATA_UNKNOWN_MODEL`).
  Name occupied by a model of another kind — `DATA_MODEL_KIND`.
- `setFilterExpression(view, xpath)`: proxy or flat tree filter by expression with `$variables` (empty — remove);
  changing a variable recalculates the filter before the next frame.
- `loadData(name, data::SourceSpec)` / `loadTree(name, spec)`: list or tree from a JSON/XML file (`path`, `format`,
  `select`, `fields`, `children`, `fetch`); error — `DataException` (file, line, column) or `GranException(IO)`.
  Without an application — `data::openListSource`, `data::loadList`, `data::readTreeSource`, `data::JsonReader`
  (`gran/data/data_source.hpp`, `gran/data/json_reader.hpp`).
- `itemOf(node)`: `ItemRef` of the closest list record (`model` — `data::ListSource*`, `index`, `selection`);
  `model == nullptr` if the node is outside the binding.
- `setVariable(name, std::string)`: the `$name` variable of the `{…}` expressions keeping its type (a new one is a string);
  `setVariableNumber(name, double)`, `setVariableBoolean(name, bool)` set the type explicitly;
  `variable(name) -> std::optional<std::string>` — the value as text (a boolean is — `"true"`/`"false"`).
- `setConverter(name, binding::Converter{toView, toSource})`: converter for `{expression ; name}`;
  empty `toSource` makes the binding one-way. An exception in the converter leaves the property unchanged.
- `refreshBindings()`: recompute the expressions now (the engine does this before every frame and after layout).
  More details — [guide](guide.en.md#binding-expressions).
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

## Windows and the program

### Owner windows and modal windows

`openWindow(const std::string& path, const WindowOptions& options = {})` and `openWindowString(std::string_view,
const WindowOptions&)` open one more window and return its content root. The `WindowOptions` fields:

- `ui::Node* owner = nullptr` — any node of the owner window. A node outside the open windows is `GranException(MARKUP)`
  (`APP_OWNER_NOT_WINDOW`), no window is created. The owned window stands in the window list after the owner, closes
  before it and together with it.
- `bool modal = false` — while the window is open, the owner and its owners get no input (Win32 `EnableWindow`,
  on X11 the engine drops the events). Without an owner the flag is reset.
- `WindowCloseHandler onClosed` — `std::function<void(const std::string& result)>`, called on the UI thread after
  the window is destroyed.

`closeWindow(ui::Node&, const std::string& result)` marks the node window for closing (after the event pass) with
a result for `onClosed`; `closeWindow(ui::Node&)` is the same with an empty result. Closing together with the owner
and the close button give `""`; closing the main window ends `run()` without calling `onClosed` of the other windows.
The built-in `gran:close:result` action is equivalent to `closeWindow(node, "result")`.

Platform: `platform::WindowDesc::owner` (`platform::Window*`), `modal`, `kind` (`WindowKind::NORMAL|DIALOG|TOOL`),
`position` (`WindowPosition::DEFAULT|CENTER|CENTER_OWNER`; the enumerations are in `gran/platform/window_frame.hpp`);
`Window::setEnabled(bool)` and `Window::activate()` (do nothing by default). Markup:
`markup::WindowSpec::kind` and `position` from the `<Window kind position>` attributes.

### The background task pool

`TaskId runTask(BackgroundWork work, TaskDone done = {}, TaskProgress progress = {})` (`gran/app/background_task.hpp`):
`BackgroundWork` is `std::function<void(TaskContext&)>` on a pool thread, `TaskDone` is `void(TaskOutcome&)` and
`TaskProgress` is `void(const TaskProgressInfo&)` on the UI thread. `cancelTask(TaskId)` sets the cancel flag (`false` —
the outcome is already delivered or the number is unknown), `activeTasks() const` is the tasks in the queue, at work and with an undelivered outcome.
`AppOptions::taskThreads` (0 — cores − 1, from 1 to 8); the threads are created on the first task.

`TaskContext` (only inside the work): `id()`, `cancelled()`, `progress(double fraction, const std::string& text = {})`
(merged to the latest value), `setResult(std::any)`, `fail(const std::string&)` — a failure without an exception.
`TaskOutcome`: `id`, `status` (`TaskStatus::DONE|CANCELLED|FAILED`), `error`, `result` (`std::any`, empty on
cancellation and failure). A work exception is `FAILED` with `what()`; an unknown exception is the `TASK_UNKNOWN_ERROR` text.
A task cancelled in the queue does not start. The application destructor cancels the tasks and waits for the pool threads (`app::TaskPool`,
the last field of `Application::State`); the outcomes not delivered by then are destroyed without calling `done`.

### The log and the crash report

`AppOptions::appName` (empty — no log), `logDirectory`, `crashReports = true`; the `Application` constructor opens
the log, writes the start and installs the crash handler. `const std::string& previousCrash() const` is the previous run report
(`.txt`; the `.seen` file in the report directory remembers the shown one).

The `gran/log/log.hpp` module (the process log, the Singleton pattern): `Level::SEVERE|WARNING|INFO|VERBOSE`; `LogOptions`
(`appName`, `directory`, `maxBytes = 1 MB`, `files = 5`, `level = INFO`); `bool open(const LogOptions&)` (again with the same
path — nothing), `close()`, `filePath()`, `directory()` (UTF-8); `write(Level, std::string_view category, std::string_view text)
noexcept` — from any thread, the file is flushed after every line, `SEVERE`/`WARNING` are duplicated to stderr;
`copyRecent(char*, std::size_t) noexcept` — the tail of the ring of the latest records (16 KB, without allocations and locks — for the crash
handler); `defaultDirectory(appName)`, `systemDescription()`.

`gran/log/crash_report.hpp`: `CrashOptions{appName, directory}`, `bool installCrashHandler(const CrashOptions&)` (again —
nothing), `crashDirectory(const CrashOptions&)`, `previousCrash(const std::string& directory)`. The file names are
`<program>-<time>-<process>.txt|.dmp|.stack`. Windows: the SEH filter hands the pointers to a helper thread (created
in advance, the crashed thread may have no stack), `MiniDumpWriteDump` (dbghelp is loaded on installing;
`MiniDumpWithIndirectlyReferencedMemory | ScanMemory | ThreadInfo | UnloadedModules`), the stack is `RtlVirtualUnwind`;
`std::terminate`, `abort`, a CRT invalid parameter are own codes `0xE0000001…3`. Linux: `sigaction` with `SA_ONSTACK | SA_RESETHAND`
on a 64 KB `sigaltstack`, only async-signal-safe calls, then the signal is repeated. Rendering: `rhi::RenderBackend::adapterInfo()`
→ `AdapterInfo{api, name, driver, software}` for the log.

### Resources inside the program

`gran/res/resources.hpp`: `Entry{path, data, size}`, `Bundle{name, entries, count}`, `ResourceView{data, size}`;
`mount(const Bundle&)` (the bytes are not copied; a bundle mounted later overrides equal paths), `mountBlob(const void*,
std::size_t)` (the "GRANRES1" block is copied; a damaged one is `GranException(RESOURCE)`, `RES_BAD_BLOB`), `unmountAll()`,
`isResourcePath(std::string_view)`, `find(std::string_view, ResourceView&)`, `normalize(std::string_view)` (both slashes,
`.` and `..`; leaving the root — empty), `readFile(const std::filesystem::path&, std::vector<std::uint8_t>&)` and
`openStream(const std::filesystem::path&)` — a resource or a disk file. `MappedFile::open("res:/…")` gives the bundle bytes without
a mapping. The block format: `"GRANRES1"`, `u32` entry count, for each `u32` path length, the path, `u64` length, the data (LE).

`gran/res/manifest.hpp` (used by `gran_rcc`): `readManifest(path)` → `Manifest{name, location, files}` (the
`gran-resources.xsd` schema; a missing file is `RES_MISSING_FILE`, a repeated path is `RES_DUPLICATE_ALIAS`), `matchPattern`, `expandFolder`,
`sortFiles`, `generateSource`, `generateBlob`, `generateDepfile`. The generated `.cpp` declares
`const gran::res::Bundle& granResources_<name>()`.

### The program settings

`settings::Settings& Application::settings()`; `AppOptions::settingsFile` (empty — `settings::defaultPath(appName)`).
`gran/settings/settings.hpp`: `ValueType::STRING|NUMBER|BOOLEAN|LIST`, `Value{type, text, items}`;
`Settings::load(path)` (no file — empty and `true`; a damaged one goes into `.bad`, `SETTINGS_DAMAGED` to the log, `false`),
`save()` (a temporary `.tmp` and renaming), `path()`, `modified()`; `getString/getNumber/getBool(name, fallback)`,
`getList`, `find(name)` → `const Value*`; `setString/setNumber/setBool/setList`, `remove`; the layout —
`windowPlacement/setWindowPlacement(layout, WindowPlacement)`, `attribute/setAttribute({layout, id, name}, value)`,
`variable/setVariable(name, Value)` (they change `modified` only for a different value).

The engine: `restorePlacement` — right after the OS window creation, before the backend (the swap chain is of the right size at once);
`restoreState` (the `persist` attributes through `setMarkupAttribute`, the `persist` variables with their type) — before the first frame;
`saveState` — before closing a window and for all the windows at the end of `run()`, then `save()` if there are changes.
Platform: `platform::WindowPlacement{x, y, width, height, maximized}`, `Window::placement()` / `setPlacement()` —
Win32 `Get/SetWindowPlacement` (`rcNormalPosition`, the maximization on `show`), X11 — the client area relative to
the root and `StaticGravity`. Markup: `WindowSpec::layout`, `data::Variable::persist`, the `persist` attribute (the `PersistList` type).

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
