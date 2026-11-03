# C++ API Грань UI

**Русский** · [English](api.en.md)

Заголовки подключаются от `include/`, например `#include "gran/app/application.hpp"`.
Пространство имён — `gran`, стандарт — C++17. Готовый пример приложения — `examples/gallery`.
Синтаксис XML и стилей описан в [руководстве](guide.md), подключение библиотеки — в [сборке](building.md).
Для Python, C# и Java предусмотрены общий [C ABI и тонкие обёртки](bindings.md).

## Application: окно и цикл событий

`Application(AppOptions options = {})` создаёт состояние; окно и GPU создаются в `run()`.
`loadFile(const std::string&)` загружает XML с проверкой схемой; пути картинок и `<Font>` отсчитываются
от каталога файла. `loadString(std::string_view)` загружает строку с ресурсами относительно текущей папки.
`run()` показывает окно после первого кадра, выполняет ввод, задачи, таймеры и анимации, возвращает код выхода.
В простое цикл спит; без загруженной разметки `run()` бросает `GranException`.

`minimize()`, `maximize()`, `restore()`, `close()` управляют окном в потоке UI (из обработчика,
таймера или задачи `post`). До создания окна команды ничего не делают. `close()` посылает обычное
событие закрытия и завершает цикл без удаления дерева внутри обработчика. `isMaximized() const`
возвращает состояние ОС; до создания окна — `false`. При `resizable="false"` команда `maximize()` пропускается.

`<Window titleBar="system">` сохраняет стандартную рамку (значение по умолчанию).
`titleBar="custom"` отдаёт всю клиентскую область разметке: `windowRole="drag|minimize|maximize|close"`
задаёт перетаскивание и команды. `maximize` переключает «развернуть/восстановить».
Кнопки ролей принимают Tab, Enter/пробел и UIA Invoke, затем вызывают свой `onClick`, если он задан.
Состояние `<Maximized>` применяется ко всему дереву, включая значок внутри кнопки. Команды доступны
в обоих режимах рамки; геометрия `drag`/Snap — только в `custom`. Параметры `titleBar` и `resizable`
создают окно: их изменение при reload вступает в силу после нового запуска.

Платформенный API: `platform::WindowDesc::titleBar`, перечисления `platform::TitleBar` и `WindowRole`
в `gran/platform/window_frame.hpp`. `Window::setFrameHitTest(std::function<WindowRole(Vec2)>)`
читает роли в потоке UI, координаты — логические пиксели клиента. `Application` подключает маршрутизатор,
учитывающий преобразования, отсечение, отключённых предков и модальные слои. Пустой обработчик отключает связь.

Поля `AppOptions`:

- `debugLayer = false`: отладочный слой GPU и инструменты WebView.
- `vsync = true`: синхронизация показа с монитором.
- `exitAfter = 0`: выход через указанное число секунд от запуска часов приложения; 0 отключает ограничение.
- `backend = rhi::BackendKind::AUTO`: D3D12 с резервным переходом на D3D11 при ошибке создания D3D12.
  `D3D11` и `D3D12` выбирают API явно: ошибка не переключает его на другой.
- `forceWarp = false`: программный адаптер WARP вместо оборудования, для воспроизводимых проверок.
- `watchMarkup = false`: наблюдение за XML-файлом с интервалом 250 мс во время `run()`.

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

## Перезагрузка разметки

`reload()` повторно загружает последний успешно загруженный файл; без источника-файла бросает
`GranException(INVALID_ARGUMENT)`. `reloadIfChanged()` проверяет время изменения и длину файла;
возвращает `true`, если изменение найдено и новый документ подготовлен, иначе `false`.
Изменение только содержимого при сохранённых времени и длине не обнаруживается: используйте `reload()`.

При `watchMarkup` проверка происходит автоматически, в том числе когда окно простаивает.
Ошибка разбора, схемы или объявленного шрифта сохраняет прежний интерфейс. Автоматический режим пишет
ошибку в stderr и `reloadError()`; успешная загрузка очищает строку. Ошибочный файл повторно проверяется
после следующего изменения. Временное отсутствие файла при атомарном сохранении редактором пропускается.
Ручные вызовы загрузки передают исключение вызывающему коду.

Внутри `run()` успешная загрузка подготавливает документ, а замена выполняется после обработки событий,
задач и таймеров. Поэтому обработчик может вызвать `loadFile`, `loadString` или `reload` без удаления
своего узла посреди вызова. Несколько загрузок до границы цикла оставляют последнюю успешную.
До `run()` замена применяется сразу. Все эти методы вызываются в потоке UI; из других потоков — через `post`.

Перезагрузка частичная. Если параметры `<Window>` и `<Resources>` (стили, тема, данные, команды) не изменились,
новый DOM сравнивается с живым: равные ветви остаются как были (те же объекты `ui::Node`, слушатели, состояние,
копии записей списков), изменённые перестраиваются на месте — сам изменившийся узел остаётся тем же объектом,
его потомки строятся заново. Добавление или удаление ребёнка перестраивает родителя. Модели не трогаются.
Иначе дерево заменяется целиком (ниже). В обоих случаях состояние пользователя — текст `Input`/`TextArea`
с кареткой и историей, отметки, значения Slider/ProgressBar, выбор ComboBox и Tabs, прокрутка `Scroll` и
фокус — переходит на новые узлы с тем же `id`, а у узлов без `id` — по месту относительно ближайшего предка с `id`.

При полной замене:

- перезагружается только главное окно; окна из `openWindow` и их узлы не меняются;
- окно ОС, бэкенд GPU, обработчики действий и WebView, задачи и таймеры сохраняются;
- шрифты `<Font>` нового документа сначала проверяются, затем добавляются в общую библиотеку шрифтов (её
  используют и другие окна, поэтому прежние шрифты из неё не удаляются);
- документ, стили, анимации, ввод, выделение, всплывающие слои, браузеры WebView и кэши рендерера создаются заново;
- модели, полученные через `model()`, сохраняются; `<Data>` нового документа заменяет данные одноимённых моделей;
- указатели и ссылки на прежние `ui::Node`, включая захваченные обработчиками, становятся недействительными:
  повторите `find(id)` после замены (при частичной замене — на узлы перестроенных ветвей);
- `loadString` отключает источник-файл; изменённые заголовок, исходный размер и другие параметры создания
  окна ОС применяются при новом запуске. Текущий размер окна сохраняется, фон и содержимое обновляются.

Перезагрузка наблюдает основной XML. Изменение отдельно взятого PNG или шрифта требует `reload()` или
повторного сохранения XML. Новый документ сначала строится и проверяется целиком, и только потом сравнивается
с живым и применяется.
Гарантия сохранения старого интерфейса относится к ошибкам загрузки; исчерпание памяти/GPU при применении
подготовленного документа может завершить цикл исключением.

Для галереи: `bin/gran_gallery samples/showcase.xml --watch --d3d11 --debug`.

## Живой документ

DOM «Берёсты» после загрузки остаётся в документе окна (`markup::UiDocument::dom`), а каждый узел, построенный
из разметки, знает свой элемент (`ui::Node::domElement`; у узлов из кода, копий записей списков, полосы
заголовков Tabs и слоя ComboBox — `nullptr`). Разметка становится источником правды, который можно менять
во время работы и сохранять:

- `markupAttribute(node, name) -> std::optional<std::string>`: атрибут элемента узла — из файла или записанный
  кодом; `nullopt` — атрибута нет или узел не из разметки.
- `setMarkupAttribute(node, name, value)`, `removeMarkupAttribute(node, name)`: записать или удалить атрибут
  в DOM и перестроить узел, как если бы так было написано в файле. Весь документ проверяется схемой; ошибка схемы
  или значения — `MarkupException`, DOM и интерфейс остаются прежними. Сам узел остаётся тем же объектом (ссылки
  на него, слушатели `addEventListener`, наведение и фокус сохраняются), его потомки строятся заново —
  ссылки на них ищите снова через `find`. Состояние потомков переходит, как при перезагрузке; запись `text`,
  `value` или `checked` меняет состояние самого узла. Дети, добавленные во время работы (`append` из кода),
  остаются на месте. Узел не из разметки — `GranException(INVALID_ARGUMENT)`.
- `appendMarkup(parent, …)` у родителя из разметки вставляет элемент и в DOM; `remove(node)` убирает его элемент.
- `saveMarkup(window = nullptr) -> std::string`, `saveMarkupFile(path, window = nullptr)`: живая разметка окна
  узла `window` (по умолчанию главного) как XML: с правками, вставками и удалениями, с отступом в два пробела,
  комментарии сохраняются. Узлы из кода и состояние (набранный текст, данные моделей) в разметку не входят.

```cpp
gran::ui::Node& panel = *app.find("sidebar");
app.setMarkupAttribute(panel, "width", "320");      // проверка схемой, перестройка панели
app.saveMarkupFile("layout.xml");                   // разметка с новой шириной
```

Уровень ниже — `gran/markup/live_document.hpp`: `editMarkupAttribute`, `rebuildFromElement`, `serializeMarkup`,
`detachMarkup`, `planPatch`. Узлы дерева окна меняет приложение (ему нужны привязки, ввод и анимации), поэтому
из кода приложения пользуйтесь методами `Application`.

## Компоненты

Компоненты объявляются в файлах-библиотеках `<Library namespace>` и подключаются `<xi:include>` в `<Resources>`
(синтаксис — в [руководстве](guide.md#компоненты)). Для кода экземпляр — обычный узел: корень развёртки с id
экземпляра; узлы тела — `find("id экземпляра.id")`.

- `setMarkupAttribute(*app.find("card"), "title", "…")` / `removeMarkupAttribute` у экземпляра — правка параметра
  или атрибута хозяина с проверкой схемой «gran-ui.xsd + схемы библиотек»; развёртка строится заново.
  Узлы тела только для чтения: `GranException(INVALID_ARGUMENT)` с текстом MARKUP_COMPONENT_INTERNAL.
- `saveMarkup` сохраняет экземпляры и `<xi:include>`, а не развёртку.
- `componentSchema(namespaceUri, window = nullptr) -> std::string` — сгенерированная XSD библиотеки
  (пусто — такой библиотеки нет). Её кладут рядом с `gran-ui.xsd` для подсказок редактора XML.
- `Options::watchMarkup` следит и за файлами библиотек: их правка перезагружает окно (частично, если пролог
  и библиотеки после правки совпадают с прежними; иначе — полностью, с переносом состояния).

Уровень ниже — `gran/markup/components.hpp`: `componentSchemas(document)` (все схемы документа: пространство имён,
файл, текст) и `libraryFiles(document)`.

## Узлы, действия и данные

`find(const std::string& id)` возвращает невладеющий `ui::Node*` или `nullptr`.
`onAction(name, ActionHandler)` регистрирует или заменяет обработчик `void(ui::Node&)` по имени из XML.
Обработчики выполняются в потоке UI. Изменения из `setValue`, `setChecked`, `selectOption`, `selectTab`
не вызывают пользовательский `onChange` повторно.

- `setStyle(node, name)`: именованный стиль текущего документа; неизвестный стиль — ошибка `RESOURCE`.
- `append(parent, std::unique_ptr<ui::Node>)`: передаёт владение ребёнком дереву, возвращает ссылку на него.
- `appendMarkup(parent, std::string_view)`: один корневой XML-фрагмент с проверкой схемой, стилями документа
  и немедленным подключением привязок списков; у родителя из разметки фрагмент входит и в живой DOM.
- `remove(node)`: удаляет поддерево (и его элемент из живого DOM), снимает ссылки ввода, анимаций и доступности.
  Корень удалять нельзя.
- `focus(ui::Node*)`: фокус из кода, `nullptr` снимает; до `run()` запрос запоминается.
- `model(name)`: ссылка на `data::ListModel` (хранилище: `insert`, `insertAll`, `append`, `remove(index[, count])`,
  `set`, `setRecord`, `move(from, count, to)`, `clear`, `setColumns`, `setFetcher`, `fetchMore`); неизвестное имя
  создаёт пустую модель.
- `list(name)`: любой список — хранилище, прокси или плоское дерево (`data::ListSource`: `size`, `at`, `set`, `columns`,
  `canFetchMore`, `fetchMore`, подписка `ListObserver` — `inserted`, `removed`, `changed`, `changedRange`, `moved`,
  `reordered`); нет — создаётся пустое хранилище.
- `tree(name)`: `data::TreeModel` (узлы — стабильные `NodeId`: `root`, `childCount`, `child`, `parent`, `row`, `record`,
  `insert`, `append`, `remove`, `move`, `set`, `clearChildren`, `appendItems` (пакетом), `setLazy` + `setFetcher(TreeFetcher)`, подписка
  `TreeObserver`); перенос узла в своё поддерево — `GranException` (`DATA_TREE_CYCLE`).
- `createProxy(name, source)` → `data::SortFilterProxy` (`setSort(parseSortKeys("…"))`, `setFilter(RecordFilter)`,
  `invalidateFilter`, `sourceIndex`, `proxyIndex`); `createTreeList(name, tree)` → `data::TreeList` (`expand`,
  `collapse`, `toggle`, `expandToDepth`, `depth`, `expanded`, `hasChildren`, `node`, `rowOf`, `setSort`,
  `setFilter(NodeFilter)`); `createSelection(name, source, mode)` → `data::SelectionModel` (`select`, `deselect`,
  `toggle`, `selectRange`, `selectAll`, `clear`, `setCurrent`, `activate(index, ctrl, shift)` — как щелчок,
  `current`, `anchor`, `isSelected`, `selected`, `count`, `version`, подписка `SelectionObserver`).
  `proxy(name)`, `treeList(name)`, `selection(name)` — существующие (нет — `GranException`, `DATA_UNKNOWN_MODEL`).
  Имя, занятое моделью другого вида, — `DATA_MODEL_KIND`.
- `setFilterExpression(view, xpath)`: фильтр прокси или плоского дерева выражением с `$переменными` (пусто — снять);
  смена переменной пересчитывает фильтр перед следующим кадром.
- `loadData(name, data::SourceSpec)` / `loadTree(name, spec)`: список или дерево из файла JSON/XML (`path`, `format`,
  `select`, `fields`, `children`, `fetch`); ошибка — `DataException` (файл, строка, столбец) или `GranException(IO)`.
  Без приложения — `data::openListSource`, `data::loadList`, `data::readTreeSource`, `data::JsonReader`
  (`gran/data/data_source.hpp`, `gran/data/json_reader.hpp`).
- `itemOf(node)`: `ItemRef` записи ближайшего списка (`model` — `data::ListSource*`, `index`, `selection`);
  `model == nullptr`, если узел вне привязки.
- `setVariable(name, std::string)`: переменная `$name` выражений `{…}` с прежним типом (новая — строка);
  `setVariableNumber(name, double)`, `setVariableBoolean(name, bool)` задают тип явно;
  `variable(name) -> std::optional<std::string>` — значение текстом (логическое — `"true"`/`"false"`).
- `setConverter(name, binding::Converter{toView, toSource})`: преобразователь для `{выражение ; name}`;
  пустой `toSource` делает привязку односторонней. Исключение из преобразователя оставляет свойство прежним.
- `refreshBindings()`: пересчитать выражения сейчас (движок и так делает это перед каждым кадром и после компоновки).
  Подробнее — [руководство](guide.md#выражения-привязки).
- `setValue(node, float)`: значение Slider/ProgressBar с ограничением границами.
- `setChecked(node, bool)`: CheckBox/Switch/RadioButton; выбор радиокнопки снимает соседей группы.
- `selectOption(comboBox, value)`: выбор по значению; неизвестное значение снимает выбор.
- `selectTab(tabs, value)`: выбор страницы; неизвестное значение сохраняет прежний выбор.
- `runCommand(id) -> bool`: выполнить `<Command id>` (окна по порядку, главное первым) от корня её окна;
  `false` — команды нет или она выключена.
- `setCommandEnabled(id, bool)`: включить или выключить команду во всех окнах вместе с элементами
  `command="id"` (состояние Disabled) и её сочетаниями; значение сохраняется для перезагрузки и новых окон.
- `commandEnabled(id) -> bool`: включена ли команда; неизвестная — `false`.
- `undoStack(name = "") -> UndoStack&`: стек отмены (`gran/undo/undo_stack.hpp`, как `QUndoStack`), создаётся
  при первом обращении: `push(unique_ptr<UndoStep>)` записывает уже выполненный шаг (или вбирает его в верхний по
  `mergeId`/`mergeWith`), `undo`, `redo`, `canUndo`, `canRedo`, `isClean`/`setClean` («документ сохранён»), `clear`,
  `count`, `index`, `setLimit` (0 — без предела), `breakMerge`. Команды `gran:undo[:name]` / `gran:redo[:name]` (без имени — сначала поле в фокусе)
  включаются по стеку сами.
- `pushUndo(undo, redo, name = "")`: записать выполненное действие парой обработчиков (`undo::ActionStep`).
- `undo(name = "") -> bool`, `redo(name = "") -> bool`: отменить / повторить шаг; `false` — нечего.
- `onUndoChanged(UndoHandler)`: обработчик `(имя стека, стек)` после каждой смены любого стека.
- Правки полей `Input`/`TextArea` идут в историю самого поля (`content.history`, Ctrl+Z/Ctrl+Y); присваивание
  `content.text` из кода делает её недействительной (сброс при следующей правке или отмене).

При прямой правке полей узла используйте `ui::markDirty(node, flags)` из `gran/ui/node.hpp`:
`DIRTY_STYLE` для пересчёта каскада, `DIRTY_LAYOUT` для компоновки, `DIRTY_PAINT` для рисования.
Не сохраняйте ссылку на удаляемый узел. Изменения дерева и моделей выполняются в потоке UI.

## События

`using EventHandler = std::function<void(ui::Node& node, ui::UiEvent& event)>` (`gran/ui/ui_event.hpp`),
`node` — узел, чей обработчик вызван (`event.current`). Всё — только в потоке UI.

- `onEvent(name, EventHandler)`: обработчик имени из атрибутов событий (`onKeyDown`, `onPointerMove`,
  `onDropCapture`, `on="своё:name"`…); заменяет прежний. Без него имя ищется среди `onAction` и `gran:…`.
- `addEventListener(node, type, EventHandler, capture = false) -> ListenerId`: слушатель у узла; `type` —
  встроенное имя (`pointerDown`, `keyUp`, `dragOver`…) или своё. Слушатель хранится в узле (`ui::Node::listeners`),
  `cloneTree` его не копирует. `removeEventListener(node, id)` — снять (можно из самого слушателя).
- `dispatchEvent(target, type, detail = "", bubbles = true) -> bool`: своё событие по маршруту захват → цель →
  всплытие; `false` — вызван `preventDefault`. Работает и до `run()` (без окна ОС).
- `capturePointer(node)` / `releasePointer(node)`: захват мыши до отпускания кнопки; до `run()` — ничего.

`ui::UiEvent`: `type` (`ui::EventType`), `name`, `target`, `current`, `related`, `phase` (1 захват, 2 цель,
3 всплытие), `position` (окно, логические пиксели), `local` (система `current`: `ui::windowToNode` минус левый
верхний угол рамки), `wheel`, `button`, `key`, `modifiers`, `clicks`, `detail`, `drag` (`format`, `data`), `dropped`,
`bubbles`, `cancelable`, `defaultPrevented`, `handled`; методы `stopPropagation()`, `stopImmediatePropagation()`,
`preventDefault()`. `ui::makeEvent(type, target, name)` строит событие с флагами как в DOM, `ui::eventName` /
`ui::eventType` переводят вид в имя и обратно, `platform::keyName(Key)` — имя клавиши.

Маршрутизация — `input::dispatchEvent(event, sink)` (`gran/input/input_router.hpp`); у своего
`input::ActionSink` её обработчики разметки идут в виртуальный `event(name, node, event)` (по умолчанию —
`action(name, node)`), а `treeVersion()` сообщает об удалении узлов (доставка прекращается).
Команды `<Command>` (`gran/ui/command.hpp`: `ui::Command`, `ui::Shortcut`, `parseShortcut`, `shortcutText`)
маршрутизатор передаёт в `shortcut(CommandQuery)` (горячая клавиша, `true` — выполнена) и `command(id, node)`
(кнопка или пункт меню с `command`); поиск по сочетанию с областями — `input::matchCommand(sources, query)`.
Загрузчик кладёт команды в `UiDocument::commands` и сразу привязывает к ним элементы своего документа.

## Задачи, таймеры и слои

`post(Task)` безопасен из других потоков: задача копируется в очередь и будит окно; выполняется
в потоке UI, после неё нужен кадр. Задачи, не выполненные до выхода, удаляются вместе с приложением.
Захватываемые объекты должны жить до выполнения задачи.

`setTimeout(double seconds, Task)` возвращает `TimerId` однократного таймера.
`setInterval(double seconds, Task)` создаёт периодический таймер (минимум 1 мс).
`clearTimer(TimerId)` снимает таймер, включая самоснятие из обработчика; неизвестный id безопасен.
Таймеры не создают потоков и срабатывают не раньше срока на очередном проходе цикла;
пропущенные периоды не догоняются. Время отсчитывается с постановки, в том числе до `run()`.

`showPopup(popup, anchor)`, `showPopupAt(popup, Vec2)` и `hidePopup(popup)` управляют всплывающими слоями.
`showDialog(dialog)` и `hideDialog(dialog)` управляют модальным слоем внутри окна.
Эти методы предназначены для работающего `run()`; диалог возвращает фокус после закрытия.

## Окна и программа

### Окна-владельцы и модальные окна

`openWindow(const std::string& path, const WindowOptions& options = {})` и `openWindowString(std::string_view,
const WindowOptions&)` открывают ещё одно окно и возвращают корень его содержимого. Поля `WindowOptions`:

- `ui::Node* owner = nullptr` — любой узел окна-владельца. Узел вне открытых окон — `GranException(MARKUP)`
  (`APP_OWNER_NOT_WINDOW`), окно не создаётся. Подчинённое окно стоит в списке окон после владельца, закрывается
  раньше него и вместе с ним.
- `bool modal = false` — пока окно открыто, владелец и его владельцы не получают ввода (Win32 `EnableWindow`,
  на X11 — отбрасывание событий движком). Без владельца флаг сбрасывается.
- `WindowCloseHandler onClosed` — `std::function<void(const std::string& result)>`, вызывается в потоке UI после
  уничтожения окна.

`closeWindow(ui::Node&, const std::string& result)` помечает окно узла к закрытию (после прохода событий) с
результатом для `onClosed`; `closeWindow(ui::Node&)` — то же с пустым результатом. Закрытие вместе с владельцем
и крестик дают `""`; закрытие главного окна завершает `run()` без вызова `onClosed` остальных окон.
Встроенное действие `gran:close:результат` равносильно `closeWindow(узел, "результат")`.

Платформа: `platform::WindowDesc::owner` (`platform::Window*`), `modal`, `kind` (`WindowKind::NORMAL|DIALOG|TOOL`),
`position` (`WindowPosition::DEFAULT|CENTER|CENTER_OWNER`; перечисления — в `gran/platform/window_frame.hpp`);
`Window::setEnabled(bool)` и `Window::activate()` (по умолчанию ничего не делают). Разметка:
`markup::WindowSpec::kind` и `position` из атрибутов `<Window kind position>`.

### Пул фоновых задач

`TaskId runTask(BackgroundWork work, TaskDone done = {}, TaskProgress progress = {})` (`gran/app/background_task.hpp`):
`BackgroundWork` — `std::function<void(TaskContext&)>` в потоке пула, `TaskDone` — `void(TaskOutcome&)` и
`TaskProgress` — `void(const TaskProgressInfo&)` в потоке UI. `cancelTask(TaskId)` ставит флаг отмены (`false` —
итог уже доставлен или номер неизвестен), `activeTasks() const` — задачи в очереди, в работе и с недоставленным итогом.
`AppOptions::taskThreads` (0 — ядра − 1, от 1 до 8); потоки создаются при первой задаче.

`TaskContext` (только внутри работы): `id()`, `cancelled()`, `progress(double fraction, const std::string& text = {})`
(сливается до последнего значения), `setResult(std::any)`, `fail(const std::string&)` — сбой без исключения.
`TaskOutcome`: `id`, `status` (`TaskStatus::DONE|CANCELLED|FAILED`), `error`, `result` (`std::any`, пуст при
отмене и сбое). Исключение работы — `FAILED` с `what()`; неизвестное исключение — текст `TASK_UNKNOWN_ERROR`.
Отменённая в очереди задача не начинается. Деструктор приложения отменяет задачи и ждёт потоки пула (`app::TaskPool`,
последнее поле `Application::State`); итоги, не доставленные к этому времени, уничтожаются без вызова `done`.

### Журнал и отчёт о сбое

`AppOptions::appName` (пусто — журнала нет), `logDirectory`, `crashReports = true`; конструктор `Application` открывает
журнал, пишет запуск и ставит обработчик сбоя. `const std::string& previousCrash() const` — отчёт прошлого запуска
(`.txt`; файл `.seen` в каталоге отчётов помнит показанный).

Модуль `gran/log/log.hpp` (журнал процесса, паттерн Singleton): `Level::SEVERE|WARNING|INFO|VERBOSE`; `LogOptions`
(`appName`, `directory`, `maxBytes = 1 МБ`, `files = 5`, `level = INFO`); `bool open(const LogOptions&)` (повторно с тем же
путём — ничего), `close()`, `filePath()`, `directory()` (UTF-8); `write(Level, std::string_view category, std::string_view text)
noexcept` — из любого потока, файл сбрасывается после каждой строки, `SEVERE`/`WARNING` дублируются в stderr;
`copyRecent(char*, std::size_t) noexcept` — хвост кольца последних записей (16 КБ, без выделений и замков — для обработчика
сбоя); `defaultDirectory(appName)`, `systemDescription()`.

`gran/log/crash_report.hpp`: `CrashOptions{appName, directory}`, `bool installCrashHandler(const CrashOptions&)` (повторно —
ничего), `crashDirectory(const CrashOptions&)`, `previousCrash(const std::string& directory)`. Имена файлов —
`<программа>-<время>-<процесс>.txt|.dmp|.stack`. Windows: фильтр SEH передаёт указатели потоку-помощнику (создан
заранее, у упавшего потока может не быть стека), `MiniDumpWriteDump` (dbghelp загружается при установке;
`MiniDumpWithIndirectlyReferencedMemory | ScanMemory | ThreadInfo | UnloadedModules`), стек — `RtlVirtualUnwind`;
`std::terminate`, `abort`, неверный параметр CRT — свои коды `0xE0000001…3`. Linux: `sigaction` с `SA_ONSTACK | SA_RESETHAND`
на `sigaltstack` 64 КБ, только async-signal-safe вызовы, затем сигнал повторяется. Рендер: `rhi::RenderBackend::adapterInfo()`
→ `AdapterInfo{api, name, driver, software}` для журнала.

## WebView

`onWebMessage(name, WebMessageHandler)` задаёт обработчик `void(ui::Node&, const std::string&)`.
`postWebMessage(node, text)` передаёт строку странице; до готовности браузера сообщения ждут в очереди.
`navigate(node, url)` меняет адрес, `loadHtml(node, html)` загружает HTML и сбрасывает `source`.
Передача обычного узла вместо WebView — ошибка. Возможности и ограничения системного браузера описаны
в [руководстве](guide.md#встроенный-браузер-webview).

## Низкоуровневая загрузка и рисование

`markup::loadMarkupFile(path, LoadOptions)` и `loadMarkupString(text, LoadOptions)` возвращают владеющий
`UiDocument`. `LoadOptions::baseDirectory` задаёт базу ресурсов, `maxNodes = 100000` ограничивает размер.
`markup::buildFragment(text, document)` возвращает отдельное дерево со стилями этого документа.
Указатели на правила стилей требуют, чтобы исходный документ жил дольше использующего их дерева.

`ui::resolveStyles` вычисляет каскад; `layout::layoutTree(root, Size, ContentMeasurer&)` размещает дерево.
`render::Renderer(RenderBackend&, FontLibrary&, imageDirectory)` предоставляет `measurer()` для компоновки
и `render(root, windowStyle, scale, caret, selection)`. Рендерер должен уничтожаться раньше бэкенда:
его деструктор возвращает текстуры. Бэкенд должен уничтожаться раньше окна ОС.

`rhi::createPlatformBackend(BackendOptions)` выбирает API; `createDx11Backend` и `createDx12Backend`
создают конкретный бэкенд (Windows 10/11). `BackendOptions` содержит `nativeWindow` (HWND или `nullptr`),
размер в физических пикселях, `vsync`, `debugLayer`, `backend` и `forceWarp`.
Без окна реализуется `SnapshotSource::readPixels(rgba, width, height)` — плотный RGBA8 сверху вниз,
ожидающий завершения GPU. `resize(0, 0)` безопасно пропускается для свёрнутого окна.

`RenderBackend` принимает `FrameData`, создаёт/обновляет/освобождает слоты текстур и отдаёт
`takeDiagnostics()` с очисткой накопленных сообщений слоя. `render()` возвращает `PRESENTED`,
`OCCLUDED` или `DEVICE_LOST`. После потери устройства GPU-кэши нужно восстановить из CPU-данных;
`Application` делает это автоматически. Все вызовы бэкенда выполняются одним потоком рендера.

D3D11 требует FL11_0/SM5, использует встроенный DXBC и фиксированные регистры SRV.
Пакеты разделяются при смене картинки; текст, градиенты, тени, маски, прозрачность и подложки используют
тот же HLSL, что D3D12. Доступно до 4095 пользовательских текстур одновременно (один слот зарезервирован).
D3D11 управляет временем жизни отправленных ресурсов и версиями динамических констант через runtime.
Размеры текстур ограничены 16384, буферы экземпляров/отсечений — 128 МиБ каждый.
Обычной программе не нужны fxc/dxc; обновление шейдеров: `make shaders-dx11` / `make shaders`.

## Ошибки и проверка снимков

`GranException::getCode()`, `getMessage()`, `what()` дают категорию и переведённое сообщение.
`MarkupException` добавляет `getLine()`/`getColumn()`, `GpuException` — `getNativeCode()`.
`i18n::setLanguage("ru"|"en")` выбирает каталог сообщений.

`render::comparePixels(actual, expected, tolerance)` сравнивает `image::Image` по RGBA8.
`PixelComparison` содержит `sameSize`, `differentPixels` (выход за допуск хотя бы одного канала)
и `maxDelta` (фактическая максимальная разница, включая допустимые).
Неверная длина массива или размер — `sameSize == false`.

[Проверки отрисовки](render_testing.md) описывают эталоны, допуски и команды Make/CMake.
