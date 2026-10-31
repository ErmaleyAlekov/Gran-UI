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

`command` поддерживает `close`, `minimize`, `maximize`, `restore`, `reload`,
`focus`, `remove`, `showDialog`, `hideDialog`, `showPopup`, `hidePopup`, `navigate`,
`loadHtml`, `postMessage`, `appendMarkup`, `capturePointer`, `releasePointer`, `runCommand`, `enableCommand`,
`disableCommand` (для этих трёх цель — id команды `<Command>`, а не узла), `undo`, `redo`, `clearUndo`,
`setUndoClean`, `setUndoLimit` (цель — имя стека отмены). Команды окна из `command` относятся к главному окну.
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

## C ABI

Все строки — UTF-8 с завершающим нулём. Возвращённые `char*` освобождаются только
`gran_string_free`, даже пустые строки. Строки входных массивов должны жить до
возврата вызова. `gran_last_error` возвращает строку потока до следующей ошибки;
успешные вызовы её не очищают. Исключения C++ перехватываются на границе ABI.

`gran_options.size` обязательно равен `sizeof(gran_options)`; `NULL` выбирает
настройки по умолчанию. `gran_abi_version()` должен возвращать 1.
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
