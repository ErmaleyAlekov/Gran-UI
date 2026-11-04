# Языковые обёртки

**Русский** · [English](bindings.en.md)

Общая библиотека `gran_c` предоставляет C ABI версии 1, описанный в
[`gran.h`](../include/gran/capi/gran.h). Python, C# и Java вызывают этот интерфейс.
Разбор XML, стили, шаблоны, привязки списков, компоновка, ввод, анимации и отрисовка
выполняются в C++. Код приложения на выбранном языке содержит обработчики и данные.

## Сборка

Windows x64: `make bindings` создаёт `bin/gran_c.dll` и использует уже собранную
«Берёсту». CMake создаёт цель `gran_c` по умолчанию; отключение:
`-DGRAN_BUILD_BINDINGS=OFF`. Библиотека использует тот же компилятор и системные
зависимости, что и движок. При включённом WebView2 загрузчик DLL должен находиться
рядом с исполняемым файлом процесса, как в C++-приложениях.

Linux x86_64: та же цель собирает `libgran_c.so` (Vulkan + X11). Python ищет библиотеку по
`GRAN_UI_LIBRARY`, C# — по `LD_LIBRARY_PATH` (имя `gran_c`), Java получает абсолютный путь к `.so`.
macOS пока не поддерживается.

## Общая модель

Приложение создаётся, загружает XML, получает обработчики и входит в `run`.
Завершение окна (`command("close")`) и освобождение приложения (`close`/`Dispose`)
— отдельные операции. Освобождение возможно после выхода из `run`.

Все операции, кроме `post`, вызываются в создавшем приложение потоке UI. Фоновый
поток передаёт изменения через `post`; пользователь гарантирует, что дескриптор
ещё существует. Нельзя одновременно освобождать приложение и отправлять задачи.

Узлы адресуются строковым `id`: ядро заново ищет узел перед каждой операцией.
После удаления или reload оставшийся в коде id безопасен: вызов сообщит, что
элемент отсутствует. Внешних `Node*` и ссылок на память дерева в ABI нет.
Адресуемые id должны быть уникальны в построенном дереве; для повторяющихся
элементов шаблона изменяйте модель через `model` и `index` события.

`read` поддерживает `text`, `value`, `checked`, `visible`, `enabled`, `option`,
`style`, `kind`. `apply` принимает массив троек `(id, свойство, значение)`;
записываются все перечисленные свойства, кроме `kind`. Число и логическое
значение передаются строками (`"42"`, `"true"`). `value` относится к Slider и
ProgressBar, `checked` — к CheckBox/Switch/RadioButton, `option` — к Tabs/ComboBox.
Для Input/TextArea текст находится в `text`. Эти изменения не вызывают onChange.

Пакет выполняется последовательно в одном вызове ядра, без промежуточного кадра.
Это не транзакция: при ошибке уже выполненные операции сохраняются. По окончании
ядро само пересчитывает помеченные ветви.

`model_insert`/`model_set` передают поля записи сразу словарём. Имена моделей
совпадают с `items="имя"` в XML. `<ItemTemplate>` и подстановки `{поле}`
обрабатывает ядро; оболочки не клонируют элементы и не интерпретируют разметку.
`model_read`, `model_count`, `model_remove`, `model_clear` работают с теми же
моделями. Привязка полей остаётся односторонней: обработчик onChange может
сохранить новое значение в модель явно.

## Ядро и язык: кто что делает

Обёртка — тонкий слой: каждый её метод — один вызов C ABI, а вся работа с интерфейсом идёт в C++. Чтобы программа
на Python, C# или Java была такой же быстрой, как на C++, язык должен говорить ядру **что** сделать, а не делать это сам.
Хорошее правило: код на языке — это реакции на действия, правила предметной области, работа с файлами, сетью и базой;
всё, что касается отображения, описано в разметке и выполняется ядром.

| Задача | Как сделать в ядре | Чего избегать в языке |
|---|---|---|
| Поменять несколько свойств | один `apply([(id, свойство, значение), …])` — один вызов, один пересчёт | цикл из отдельных вызовов на каждое свойство |
| Показать список записей | модель + `items="имя"` и `<ItemTemplate>`; `model_insert` / `model_set` | создание элементов через `appendMarkup` для каждой записи |
| Загрузить данные из файла | `load_data` / `load_tree`: JSON и XML разбирает ядро | разбор файла в языке и вставка по одной записи |
| Сортировка и поиск | `create_proxy`, `set_sort`, `set_filter` (XPath) — источник не меняется | пересортировка и пересборка модели при каждом вводе в поиск |
| Видимость, доступность, тексты от состояния | выражения привязки и переменные: один `set_variable`, ядро пересчитывает все зависимые атрибуты | обход узлов и `apply` после каждой смены режима |
| Открыть диалог, меню, закрыть окно, отмена | команды `gran:` прямо в разметке | обработчик на языке, который только вызывает `command` |
| Анимации и плавные переходы | `transition` и ключевые кадры в стилях | таймер языка, меняющий свойство каждый кадр |
| Большие списки и деревья | `on_fetch` и ленивые узлы — данные по мере прокрутки и раскрытия | вставка всех записей заранее |
| Следить за состоянием | события: onChange, `on_selection_changed`, `on_undo_changed` | опрос `read` по таймеру |

**Обработчик должен возвращаться быстро.** Он выполняется в потоке UI: пока он работает, окно не рисуется и не отвечает.
Всё, что дольше нескольких миллисекунд (файлы, сеть, база, расчёты), — в `run_task`: работа идёт в потоке пула,
`task.progress` двигает полосу прогресса, `done` получает результат уже в потоке UI. Свой поток тоже можно,
тогда изменения передаются в интерфейс только через `post`.

**Преобразователи вызывают язык из ядра** при каждом вычислении привязки. Держите их короткими и без побочных
действий; простое форматирование и арифметику лучше записать выражением XPath прямо в разметке.

**Модель — для отображения, объекты предметной области — в языке.** Привязка полей односторонняя: onChange
сохраняет новое значение в объект языка и, если нужно, в модель через `model_set`. Модель не заменяет базу данных
и не должна быть единственным хранилищем бизнес-данных.

**Особенности языков.**
- Python: `ctypes` отпускает GIL на время вызова C, поэтому пока `run` ждёт событий и рисует кадр, другие потоки Python
  работают. Но тяжёлый расчёт на чистом Python в `run_task` держит GIL, и обработчики UI ждут его. Для таких расчётов —
  отдельный процесс (`multiprocessing`, `concurrent.futures.ProcessPoolExecutor`) или библиотека, которая отпускает GIL
  (например, NumPy); результат возвращается в интерфейс через `post`.
- C# и Java: кадр рисует C++, пока поток UI находится в нативном коде, поэтому пауза сборщика мусора не останавливает
  анимации — она задерживает только ответ обработчика. Не создавайте в частых обработчиках (движение мыши, таймеры)
  много временных объектов.
- Любой язык: обработчики, преобразователи и задачи `post` удерживаются обёрткой до `close` (или до снятия/исполнения),
  поэтому замыкания не должны держать большие данные дольше нужного.

Пример (Python): поиск по списку без обхода записей в языке.

```python
# Разметка: <Input id="search" onChange="search"/> и список с items="visible".
app.load_data("files", "files.json", select="files")         # разбор JSON — в ядре
app.create_proxy("visible", "files", sort="name")             # сортировка и фильтр — в ядре

def on_search(event):
    # Язык только переводит ввод в условие; отбор строк делает ядро.
    text = event.text.replace("'", "")
    app.set_filter("visible", f"contains(name, '{text}')")

app.on_action("search", on_search)
```

## События и команды

Именованные обработчики `on_action` соответствуют onClick/onChange/onSubmit,
`on_message` — сообщениям WebView. Событие содержит `target`, `model`, `index`,
`text`, `option`, `message`, `value`, `checked`. Для элемента шаблона `model` и
`index` указывают запись, даже если у дочернего элемента нет id. Вне списка
нативный `index` равен `SIZE_MAX`. Строки копируются в управляемую память до
передачи пользовательскому обработчику.

Исключения обработчиков остаются в языке приложения: обёртка запоминает первое,
запрашивает закрытие окна и повторно выбрасывает исключение после `run`.
Обработчики `onAction`/`onMessage` удерживаются до освобождения приложения. Задачи `post` и таймеры
идут через один общий переходник: одноразовая задача освобождается после вызова, таймер — после
срабатывания (`repeat=false`) или `clear_timer`, поэтому частые `post` из рабочего потока не копят память.
Число ждущих задач — `pending_tasks()` / `PendingTasks` / `pendingTasks()`.
Таймеры и фоновые задачи исполняются в потоке UI.

### События интерфейса

Атрибуты событий (`onPointerDown`, `onKeyDownCapture`, `onDrop`, `on="своё:имя"`, см. «События» в
[руководстве](guide.md#события)) связываются с языком так же по имени:

| Python | C# | Java | C ABI |
|---|---|---|---|
| `on_event(name, f)` | `OnEvent(name, f)` | `onEvent(name, f)` | `gran_app_on_event` |
| `listen(id, type, f, capture)` → номер | `Listen(...)` | `listen(...)` | `gran_app_listen` |
| `unlisten(id, номер)` | `Unlisten` | `unlisten` | `gran_app_unlisten` |
| `dispatch(id, type, detail)` → `bool` | `Dispatch` | `dispatch` | `gran_app_dispatch` |
| `capture_pointer(id)` / `release_pointer(id)` | `CapturePointer` / `ReleasePointer` | `capturePointer` / `releasePointer` | `command("capturePointer", id)` |
| `run_command(id)` | `RunCommand(id)` | `runCommand(id)` | `command("runCommand", id)` |
| `set_command_enabled(id, bool)` | `SetCommandEnabled(id, bool)` | `setCommandEnabled(id, bool)` | `command("enableCommand" / "disableCommand", id)` |

Обработчик получает снимок `gran_input_event`: `type`, `target`, `current`, `related` (id узлов),
`phase` (1 захват, 2 цель, 3 всплытие), `x`/`y` в системе `current`, `window_x`/`window_y`, `wheel_x`/`wheel_y`,
`button`, `clicks`, `modifiers` (1 Shift, 2 Ctrl, 4 Alt), `key` («A», «Enter», «F5»), `detail`, `drag_format`,
`drag_data`, `dropped`, `model`/`index` узла `current` и `related_model`/`related_index` узла `related`
(перетаскиваемая карточка списка — сразу её запись). Методы снимка `prevent_default()` / `PreventDefault()` /
`preventDefault()`, `stop_propagation()` и `stop_immediate_propagation()` ставят флаги, которые уходят ядру
после возврата обработчика (в C ABI — возвращаемое значение `GRAN_EVENT_*`). Слушатель `listen` живёт
в узле и снимается вместе с ним; `dispatch` работает и до `run`.

### Отмена и повтор

Стеки отмены приложения (см. «Отмена и повтор» в [руководстве](guide.md#отмена-и-повтор)); имя стека
`None`/`null` — основной. Шаг кладётся уже выполненным: обёртка хранит пару функций и отпускает её, когда
стек отбросил шаг (новый шаг поверх отменённых, предел, `clear_undo`, закрытие приложения). Поля `Input`
и `TextArea` ведут свои истории (Ctrl+Z в поле) и в эти стеки не пишут.

| Python | C# | Java | C ABI |
|---|---|---|---|
| `push_undo(undo, redo, stack)` | `PushUndo(undo, redo, stack)` | `pushUndo(undo, redo, stack)` | `gran_app_undo_push` |
| `undo(stack)` / `redo(stack)` | `Undo` / `Redo` | `undo` / `redo` | `command("undo" / "redo", stack)` |
| `undo_state(stack)` → `UndoState(can_undo, can_redo, clean)` | `GetUndoState` → `UndoState` | `undoState` → `UndoState` | `gran_app_undo_state` (флаги `GRAN_UNDO_*`) |
| `set_undo_clean(stack)` | `SetUndoClean` | `setUndoClean` | `command("setUndoClean", stack)` |
| `clear_undo(stack)` | `ClearUndo` | `clearUndo` | `command("clearUndo", stack)` |
| `set_undo_limit(n, stack)` | `SetUndoLimit` | `setUndoLimit` | `command("setUndoLimit", stack, "n")` |
| `on_undo_changed(f(stack, state))` | `OnUndoChanged` | `onUndoChanged` | `gran_app_on_undo` |

Команды `<Command action="gran:undo">` / `gran:redo` (`gran:undo:имя` — свой стек) включаются по стеку сами;
`on_undo_changed` удобен для «*» в заголовке (`not state.clean`).

### Живой документ

Разметка после загрузки остаётся живой (см. [API](api.md#живой-документ)): атрибут узла можно прочитать, записать
с проверкой схемой и перестройкой узла (состояние полей внутри сохраняется) и сохранить разметку окна в XML.
Ошибка схемы — исключение языка (`RuntimeError`, `InvalidOperationException`, `IllegalStateException`), разметка прежняя.

| Python | C# | Java | C ABI |
|---|---|---|---|
| `markup_attribute(target, name)` → `str` или `None` | `MarkupAttribute` → `string?` | `markupAttribute` → `String` или `null` | `gran_app_markup_read` (`*result == NULL` — нет) |
| `set_markup_attribute(target, name, value)` (`None` — удалить) | `SetMarkupAttribute` | `setMarkupAttribute` | `gran_app_markup_write` (`value == NULL` — удалить) |
| `save_markup(window)` → XML | `SaveMarkup` | `saveMarkup` | `gran_app_markup_save` |

### Компоненты

Экземпляры компонентов (см. [руководство](guide.md#компоненты)) правятся теми же `set_markup_attribute`,
узлы тела ищутся по id «экземпляр.id». Сгенерированная XSD библиотеки — для подсказок редактора XML:

| Python | C# | Java | C ABI |
|---|---|---|---|
| `component_schema(namespace, window=None)` → `str` (`""` — нет) | `ComponentSchema(namespaceUri, window)` | `componentSchema(namespaceUri[, window])` | `gran_app_component_schema` |

### Выражения привязки

Переменные `$имя` и преобразователи `{выражение ; имя}` выражений разметки (см. [руководство](guide.md#выражения-привязки)).
Значение bool / число / строка выбирает тип переменной; строка сохраняет прежний тип (новая переменная — строка).
Преобразователь — функция «строка → строка»; без `to_source` привязка односторонняя. Исключение в преобразователе
обрабатывается как в любом обработчике: приложение закрывается, ошибка выбрасывается из `run`.

| Python | C# | Java | C ABI |
|---|---|---|---|
| `set_variable(name, value)` (`bool`, `int`/`float`, `str`) | `SetVariable(name, bool/double/string)` | `setVariable(name, boolean/double/String)` | `gran_app_variable_set_boolean/_number/_text` |
| `variable(name)` → `str` или `None` | `Variable` → `string?` | `variable` → `String` или `null` | `gran_app_variable_read` (`*result == NULL` — нет) |
| `set_converter(name, to_view, to_source=None)` | `SetConverter(name, toView, toSource)` | `setConverter(name, toView, toSource)` | `gran_app_converter` + `gran_converted_set` |
| `refresh_bindings()` | `RefreshBindings()` | `refreshBindings()` | `gran_app_refresh_bindings` |

### Модели: деревья, представления, выделение

Таблицы, деревья, прокси сортировки и фильтра, выделение и источники JSON/XML (см. [руководство](guide.md#модели-таблицы-деревья-сортировка-выделение)).
Имена общие с разметкой. `model_count`/`model_read`/`model_set` работают с любым списком (прокси пишет в источник).
Узлы дерева — стабильные номера, 0 — верхний уровень. Подгрузчик `on_fetch` сам вставляет записи
`model_insert` и сообщает конец `set_more(model, False)`. Обработчики живут до `close`.

| Python | C# | Java | C ABI |
|---|---|---|---|
| `model_move(model, start, count, to)` | `ModelMove` | `modelMove` | `gran_model_move` |
| `model_columns(model, [(name, title, "number")])` | `ModelColumns(model, Column[])` | `modelColumns(model, columns, types)` | `gran_model_columns` (`GRAN_COLUMN_*`) |
| `load_data(model, path, select, fetch, fields)` | `LoadData` | `loadData` | `gran_model_load` (`gran_source`) |
| `on_fetch(model, f(model))` / `set_more` / `fetch_more` | `OnFetch` / `SetMore` / `FetchMore` | `onFetch` / `setMore` / `fetchMore` | `gran_model_on_fetch` / `_set_more` / `_fetch_more` |
| `tree_insert(tree, parent, fields, index)` → номер | `TreeInsert` → `ulong` | `treeInsert` → `long` | `gran_tree_insert` |
| `tree_remove` / `tree_move` / `tree_set` / `tree_read` | `TreeRemove` / `TreeMove` / `TreeSet` / `TreeRead` | `treeRemove` / `treeMove` / `treeSet` / `treeRead` | `gran_tree_remove/_move/_set/_read` |
| `tree_children(tree, node)` → список номеров, `tree_parent` | `TreeChildren`, `TreeParent` | `treeChildren`, `treeParent` | `gran_tree_children` + `gran_tree_child`, `gran_tree_parent` |
| `tree_set_lazy(tree, node)`, `on_tree_fetch(tree, f(tree, node))` | `TreeSetLazy`, `OnTreeFetch` | `treeSetLazy`, `onTreeFetch` | `gran_tree_set_lazy`, `gran_tree_on_fetch` |
| `load_tree(tree, path, select, children)` | `LoadTree` | `loadTree` | `gran_tree_load` |
| `create_proxy(name, source, sort, filter)` | `CreateProxy` | `createProxy` | `gran_view_create` (`GRAN_VIEW_SORT_FILTER`) |
| `create_tree_list(name, tree, sort, filter, expanded)` | `CreateTreeList` | `createTreeList` | `gran_view_create` (`GRAN_VIEW_TREE_LIST`) |
| `create_selection(name, source, mode)` | `CreateSelection(name, source, SelectionMode)` | `createSelection(name, source, mode)` | `gran_view_create` (`GRAN_VIEW_SELECTION`) |
| `set_sort(view, keys)` / `set_filter(view, xpath)` | `SetSort` / `SetFilter` | `setSort` / `setFilter` | `gran_view_sort` / `gran_view_filter` |
| `source_index(view, row)` | `SourceIndex` | `sourceIndex` | `gran_view_source_index` |
| `tree_row(view, row)` → `{node, depth, expanded, has_children}`, `tree_expand(view, row, "toggle")` | `TreeRowOf` → `TreeRow`, `TreeExpand` | `treeRow` → `TreeRow`, `treeExpand` | `gran_tree_row_read`, `gran_tree_expand` |
| `selection_change(name, "range", index, last)` | `SelectionChange(name, SelectionCommand, index, last)` | `selectionChange(name, command, index, last)` | `gran_selection_change` (`GRAN_SELECTION_*`) |
| `selection(name)` → `(current или None, [номера])` | `Selection` → `(Current, Selected)` | `selection` → `SelectionState` | `gran_selection_read` |
| `on_selection_changed(name, f(name))` | `OnSelectionChanged` | `onSelectionChanged` | `gran_selection_on_change` |

`command` поддерживает `close`, `minimize`, `maximize`, `restore`, `reload`,
`focus`, `remove`, `showDialog`, `hideDialog`, `showPopup`, `hidePopup`, `navigate`,
`loadHtml`, `postMessage`, `appendMarkup`, `capturePointer`, `releasePointer`, `runCommand`, `enableCommand`,
`disableCommand` (для этих трёх цель — id команды `<Command>`, а не узла), `undo`, `redo`, `clearUndo`,
`setUndoClean`, `setUndoLimit` (цель — имя стека отмены), `setVariable` (цель — имя переменной, аргумент — значение),
`toggleVariable` (переключает логическую переменную). В разметке есть ещё `gran:expand`, `gran:collapse`, `gran:toggle`
(строка плоского дерева, у элемента внутри копии шаблона). Команды окна из `command` относятся к главному окну.
Для `showPopup` аргумент — id якоря;
для `appendMarkup` — XML-фрагмент, который ядро проверяет общей XSD.

В разметке те же команды доступны без обработчика языка:

```xml
<Button text="Открыть" onClick="gran:showDialog:settings"/>
<Button text="Закрыть" onClick="gran:hideDialog:settings"/>
<Button text="Выход" onClick="gran:close"/>
```

Формат: `gran:команда[:id[:аргумент]]`. Префикс `gran:` зарезервирован ядром.
Для `showPopup` без аргумента якорем служит сама нажатая кнопка (id ей не нужен).
`close`, `minimize`, `maximize`, `restore` без id действуют на окно, где нажата кнопка (в том числе на окно
из `openWindow`). Ошибка в команде разметки (неизвестный id, неверный XML при `reload`) не завершает
приложение: текст ошибки из каталога пишется в stderr.
Обычные имена действий вызывают зарегистрированную бизнес-логику.

### Окна и программа

Окна-владельцы, модальные окна, фоновые задачи, журнал, ресурсы и настройки (см. [руководство](guide.md#окна-и-программа)). Работа фоновой задачи
выполняется в потоке пула: исключение языка внутри неё становится сбоем (`failed`) с текстом, результат — объект языка,
который обёртка хранит сама до `done`. Владелец — id любого узла его окна;
обработчик закрытия получает результат (`""` — крестик или закрытие с владельцем) и вызывается один раз в потоке UI.

| Python | C# | Java | C ABI |
|---|---|---|---|
| `open_window(source, is_file=True, owner=None, modal=False, on_closed=None)` | `OpenWindow(source, isFile, owner, modal, onClosed)` | `openWindow(source, isFile, owner, modal, onClosed)` / `openWindow(path)` | `gran_app_open_window` (`gran_window_options`) |
| `close_window(target, result=None)` | `CloseWindow(target, result)` | `closeWindow(target, result)` | `gran_app_close_window` |
| `run_task(work, done, progress)` → номер; `work(task)` возвращает результат, `task.cancelled()`, `task.progress(f, text)`; `done(status, result, error)`, `status` — `"done"`/`"cancelled"`/`"failed"` | `RunTask(work, done, progress)`; `BackgroundTask.Cancelled/Progress`; `BackgroundStatus` | `runTask(work, done, progress)`; `BackgroundTask.cancelled/progress`; `TASK_DONE/CANCELLED/FAILED` | `gran_app_run_task` (`gran_background`), `gran_task_cancelled/_progress/_fail` |
| `cancel_task(id)` → `bool` | `CancelTask` | `cancelTask` | `gran_app_cancel_task` |
| `open_log(app_name, directory=None, crash_reports=True)`, `log("info", category, text)`, `log_path()`, `previous_crash()` | `OpenLog`, `Log(LogLevel, …)`, `LogPath`, `PreviousCrash` | `openLog`, `log(LOG_INFO, …)`, `logPath`, `previousCrash` | `gran_log_open` / `_write` (`GRAN_LOG_*`) / `_path` / `_previous_crash` (на весь процесс) |
| `mount_resources(bytes или путь)` — блок `gran_rcc --blob` | `MountResources(byte[])` | `mountResources(byte[])` | `gran_resources_mount` (байты копируются) |
| `load_settings(app_name=None, path=None)`, `setting(name, default)` → `str`/`float`/`bool`/`list`, `set_setting(name, value)` (тип — по значению), `remove_setting`, `save_settings` | `LoadSettings`, `GetSetting` → `object?`, `SetSetting` (string/double/bool/список), `RemoveSetting`, `SaveSettings` | `loadSettings`, `getSetting` → `Object`, `setSetting`, `removeSetting`, `saveSettings` | `gran_settings_load/_get/_set/_list_item/_set_list/_remove/_save` (`GRAN_SETTING_*`) |

## C ABI

Все строки — UTF-8 с завершающим нулём. Возвращённые `char*` освобождаются только
`gran_string_free`, даже пустые строки. Строки входных массивов должны жить до
возврата вызова. `gran_last_error` возвращает строку потока до следующей ошибки;
успешные вызовы её не очищают. Исключения C++ перехватываются на границе ABI.

`gran_options.size` обязательно равен `sizeof(gran_options)`; `NULL` выбирает
настройки по умолчанию. `gran_abi_version()` должен возвращать `GRAN_ABI_VERSION` (сейчас 1);
`gran_version_string()` и `gran_version_number()` сообщают версию собранной библиотеки
(правила версий — [versioning.md](versioning.md)).
Коды: `GRAN_OK`, `GRAN_ERROR`, `GRAN_INVALID_ARGUMENT`, `GRAN_WRONG_THREAD`,
`GRAN_BUSY`. Пользовательские C callbacks не должны выбрасывать исключения.
Их функция и user-данные действительны до снятия/замены или уничтожения приложения;
для отправленной задачи — до исполнения либо уничтожения приложения.
Сама DLL остаётся загруженной до завершения процесса: зарегистрированные классы
Win32 и службы COM могут пережить отдельное приложение. Java кэширует загрузку
библиотеки в глобальной арене, а арены callbacks освобождает после уничтожения
приложения; повторное создание Application в той же JVM поддерживается.

Python использует стандартный `ctypes`, C# — P/Invoke (.NET 9), Java — стандартный
Foreign Function & Memory API (JDK 22+, проверка на JDK 25; 64-битный процесс).
Не нужны JNA, JNI-мост, Python extension или сторонние пакеты.

## Запуск примеров и проверок

Команды выполняются из корня проекта после `make bindings`.

Python 3: `python bindings/examples/python/counter.py`.
Можно задать `GRAN_UI_LIBRARY` или аргумент `library` при создании Application.
Проверки: `python bindings/tests/test_python.py -v`.

C#: добавьте `bin` в PATH на Windows, затем
`dotnet run --project bindings/examples/csharp/Counter.csproj`.
Проверки: `dotnet run --project bindings/tests/csharp/Smoke.csproj`.
В собственном проекте добавьте ProjectReference на `bindings/csharp/GranUI.csproj`
и разместите нативную DLL в каталоге программы либо в пути загрузки ОС.

Java: `javac -d bin/java bindings/java/src/granui/Application.java bindings/examples/java/Counter.java`,
затем `java --enable-native-access=ALL-UNNAMED -cp bin/java Counter <абсолютный-путь-к-gran_c.dll>`.
Проверки: скомпилируйте вместо Counter файл `bindings/tests/java/Smoke.java`
и запустите класс Smoke с тем же аргументом DLL. Для именованного модуля вместо
ALL-UNNAMED укажите имя своего модуля.

Все примеры используют один `bindings/examples/counter.xml`. Обработчик `add`
только добавляет запись модели; кнопки диалога работают через встроенные команды.
Нативные проверки C API входят в `make test`. Живые тесты языков используют
D3D11/WARP, настоящие события клавиатуры, сборку мусора, post, таймеры и исключения
callbacks; C# и Java дополнительно проверяют несколько Application в одном процессе.
