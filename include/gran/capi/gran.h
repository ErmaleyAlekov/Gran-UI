#ifndef GRAN_CAPI_GRAN_H
#define GRAN_CAPI_GRAN_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) && defined(GRAN_CAPI_BUILD)
# define GRAN_API __declspec(dllexport)
#elif defined(_WIN32) && defined(GRAN_CAPI_SHARED)
# define GRAN_API __declspec(dllimport)
#elif defined(__GNUC__)
# define GRAN_API __attribute__((visibility("default")))
#else
# define GRAN_API
#endif
#ifdef __cplusplus
extern "C" {
#endif

/* Все строки — UTF-8 с завершающим нулём. ABI 1; только 64-битные идентификаторы таймеров.
 * All strings are null-terminated UTF-8. ABI 1; timer identifiers are always 64-bit. */
typedef struct gran_app gran_app;
typedef int32_t gran_status;
enum { GRAN_OK = 0, GRAN_ERROR = 1, GRAN_INVALID_ARGUMENT = 2, GRAN_WRONG_THREAD = 3, GRAN_BUSY = 4 };

/* Нулевые параметры выбирают настройки по умолчанию. backend: 0 auto, 1 D3D11, 2 D3D12, 3 Vulkan (Linux).
 * Zero parameters select defaults. backend: 0 auto, 1 D3D11, 2 D3D12, 3 Vulkan (Linux). */
typedef struct gran_options {
    uint32_t size;
    int32_t backend;
    int32_t debug_layer;
    int32_t force_warp;
    int32_t watch_markup;
    int32_t no_vsync;
    double exit_after;
} gran_options;

/* Пакет выполняется последовательно в ядре; при ошибке предыдущие операции сохраняются.
 * A batch runs sequentially in the core; earlier operations remain applied on error. */
typedef struct gran_update {
    const char* target;
    const char* property;
    const char* value;
} gran_update;

typedef struct gran_field {
    const char* name;
    const char* value;
} gran_field;

/* Снимок события действителен только внутри callback. index == SIZE_MAX означает «вне списка».
 * The event snapshot is valid only inside callback. index == SIZE_MAX means "outside a list". */
typedef struct gran_event {
    const char* target;
    const char* model;
    size_t index;
    const char* text;
    const char* option;
    const char* message;
    double value;
    int32_t checked;
} gran_event;
typedef void (*gran_action_fn)(void* user, const gran_event* event);
typedef void (*gran_task_fn)(void* user);
typedef struct gran_timer {
    double seconds;
    int32_t repeat;
    gran_task_fn callback;
    void* user;
} gran_timer;
typedef struct gran_record {
    size_t index;
    const gran_field* fields;
    size_t count;
} gran_record;

/* Все вызовы, кроме post, — в потоке создания. destroy запрещён внутри run/callback.
 * All calls except post belong to the creating thread. destroy is forbidden inside run/callback.
 * Нельзя выбрасывать исключения из callback; user и callback живут до замены/снятия или destroy.
 * Callbacks must not throw; user and callback live until replacement/cancellation or destroy. */
GRAN_API uint32_t gran_abi_version(void);
GRAN_API const char* gran_last_error(void);
GRAN_API void gran_string_free(char* text);
GRAN_API gran_status gran_app_create(const gran_options* options, gran_app** result);
GRAN_API gran_status gran_app_destroy(gran_app* app);
GRAN_API gran_status gran_app_load(gran_app* app, const char* source, int32_t is_file);
GRAN_API gran_status gran_app_run(gran_app* app, int32_t* exit_code);
GRAN_API gran_status gran_app_on_action(gran_app* app, const char* name, gran_action_fn callback, void* user);
GRAN_API gran_status gran_app_on_message(gran_app* app, const char* name, gran_action_fn callback, void* user);
GRAN_API gran_status gran_app_post(gran_app* app, gran_task_fn callback, void* user);
GRAN_API gran_status gran_app_timer(gran_app* app, const gran_timer* timer, uint64_t* result);
GRAN_API gran_status gran_app_clear_timer(gran_app* app, uint64_t timer);

/* property: text, value, checked, visible, enabled, option, style; чтение также kind.
 * property: text, value, checked, visible, enabled, option, style; reading also supports kind.
 * read возвращает выделенную строку; освободить gran_string_free, в том числе пустую.
 * read returns an allocated string; free it with gran_string_free, including an empty string. */
GRAN_API gran_status gran_app_read(gran_app* app, const char* target, const char* property, char** result);
GRAN_API gran_status gran_app_apply(gran_app* app, const gran_update* updates, size_t count);

/* command: close/minimize/maximize/restore/reload/focus/remove/showDialog/hideDialog/
 * showPopup/hidePopup/navigate/loadHtml/postMessage/appendMarkup/capturePointer/releasePointer,
 * runCommand/enableCommand/disableCommand (target — id команды), undo/redo/clearUndo/setUndoClean/setUndoLimit
 * (target — имя стека отмены). target и argument могут быть NULL.
 * command: close/minimize/maximize/restore/reload/focus/remove/showDialog/hideDialog/
 * showPopup/hidePopup/navigate/loadHtml/postMessage/appendMarkup/capturePointer/releasePointer,
 * runCommand/enableCommand/disableCommand (target is a command id), undo/redo/clearUndo/setUndoClean/setUndoLimit
 * (target is an undo stack name). target and argument may be NULL. */
GRAN_API gran_status gran_app_command(gran_app* app, const char* command, const char* target, const char* argument);

/* События интерфейса (onPointerDown="имя", on="своё:имя", слушатели). Снимок действителен только внутри
 * callback; size — размер структуры (новые поля добавляются в конец). phase: 1 захват, 2 цель, 3 всплытие.
 * button: 0 левая, 1 правая, 2 средняя; modifiers: 1 Shift, 2 Ctrl, 4 Alt; key — имя клавиши ("A", "Enter", "F5").
 * x, y — точка в системе узла current (от его левого верхнего угла); window_x, window_y — в окне (логические
 * пиксели). related — откуда/куда ушли указатель или фокус, источник перетаскивания у зоны, зона у dragEnd.
 * index и related_index == SIZE_MAX — узел не из списка. Возврат callback — флаги GRAN_EVENT_*.
 * UI events (onPointerDown="name", on="custom:name", listeners). The snapshot is valid only inside
 * callback; size is the structure size (new fields are appended). phase: 1 capture, 2 target, 3 bubbling.
 * button: 0 left, 1 right, 2 middle; modifiers: 1 Shift, 2 Ctrl, 4 Alt; key is the key name ("A", "Enter", "F5").
 * x, y is the point in the current node system (from its top left corner); window_x, window_y — in the window (logical
 * pixels). related is where the pointer or the focus came from/went to, the drag source for a zone, the zone for dragEnd.
 * index and related_index == SIZE_MAX — the node is not from a list. The callback returns GRAN_EVENT_* flags. */
typedef struct gran_input_event {
    uint32_t size;
    int32_t phase;
    const char* type;
    const char* target;
    const char* current;
    const char* related;
    const char* key;
    const char* detail;
    const char* drag_format;
    const char* drag_data;
    const char* model;
    const char* related_model;
    size_t index;
    size_t related_index;
    double x;
    double y;
    double window_x;
    double window_y;
    double wheel_x;
    double wheel_y;
    int32_t button;
    int32_t clicks;
    int32_t modifiers;
    int32_t dropped;
} gran_input_event;
enum { GRAN_EVENT_PREVENT_DEFAULT = 1, GRAN_EVENT_STOP_PROPAGATION = 2, GRAN_EVENT_STOP_IMMEDIATE = 4 };
typedef int32_t (*gran_event_fn)(void* user, const gran_input_event* event);

/* on_event — обработчик имени из разметки (NULL снимает, тогда имя ищется среди on_action); listen —
 * слушатель type у узла target (capture != 0 — фаза захвата), снимается unlisten или удалением узла;
 * dispatch — своё событие из кода, *allowed (может быть NULL) = 0, если обработчик отменил действие.
 * on_event is the handler of a markup name (NULL clears it, then the name is looked for among on_action); listen is
 * a listener of type at the target node (capture != 0 — the capture phase), removed by unlisten or by removing the node;
 * dispatch is a custom event from code, *allowed (may be NULL) = 0 if a handler cancelled the action. */
GRAN_API gran_status gran_app_on_event(gran_app* app, const char* name, gran_event_fn callback, void* user);
GRAN_API gran_status gran_app_listen(gran_app* app, const char* target, const char* type, int32_t capture,
    gran_event_fn callback, void* user, uint64_t* result);
GRAN_API gran_status gran_app_unlisten(gran_app* app, const char* target, uint64_t listener);
GRAN_API gran_status gran_app_dispatch(gran_app* app, const char* target, const char* type, const char* detail,
    int32_t* allowed);
GRAN_API gran_status gran_model_count(gran_app* app, const char* model, size_t* result);
GRAN_API gran_status gran_model_read(gran_app* app, const char* model, size_t index, const char* field, char** result);
/* insert: index == SIZE_MAX добавляет в конец; set меняет поля одной записи за вызов.
 * insert: index == SIZE_MAX appends; set changes fields of one record in one call. */
GRAN_API gran_status gran_model_insert(gran_app* app, const char* model, const gran_record* record);
GRAN_API gran_status gran_model_set(gran_app* app, const char* model, const gran_record* record);
GRAN_API gran_status gran_model_remove(gran_app* app, const char* model, size_t index);
GRAN_API gran_status gran_model_clear(gran_app* app, const char* model);

/* Стеки отмены приложения по имени (NULL или "" — основной; поля Input/TextArea ведут свои истории).
 * undo_push записывает уже выполненное действие: undo/redo вызываются при отмене и повторе (новые шаги
 * из них не записываются), release (может быть NULL) — один раз, когда стек отбросил шаг (новый шаг поверх
 * отменённых, предел, clearUndo, destroy). undo_state — флаги GRAN_UNDO_*. on_undo — обработчик смены
 * любого стека (имя стека и флаги); NULL снимает. Отмена, повтор, «чистое» состояние и предел — через
 * gran_app_command: undo, redo, clearUndo, setUndoClean (target — имя стека), setUndoLimit (argument — число шагов,
 * "0" — без предела).
 * The application undo stacks by name (NULL or "" is the main one; Input/TextArea fields keep their own histories).
 * undo_push records an already done action: undo/redo are called on undo and redo (new steps
 * from them are not recorded), release (may be NULL) is called once when the stack dropped the step (a new step over
 * undone ones, the limit, clearUndo, destroy). undo_state is GRAN_UNDO_* flags. on_undo is a handler of a change
 * of any stack (the stack name and the flags); NULL clears it. Undo, redo, the clean state and the limit go through
 * gran_app_command: undo, redo, clearUndo, setUndoClean (target is the stack name), setUndoLimit (argument is the step count,
 * "0" — no limit). */
enum { GRAN_UNDO_CAN_UNDO = 1, GRAN_UNDO_CAN_REDO = 2, GRAN_UNDO_CLEAN = 4 };
typedef struct gran_undo_step {
    uint32_t size;
    gran_task_fn undo;
    gran_task_fn redo;
    gran_task_fn release;
    void* user;
} gran_undo_step;
typedef void (*gran_undo_fn)(void* user, const char* stack, uint32_t flags);
GRAN_API gran_status gran_app_undo_push(gran_app* app, const char* stack, const gran_undo_step* step);
GRAN_API gran_status gran_app_undo_state(gran_app* app, const char* stack, uint32_t* flags);
GRAN_API gran_status gran_app_on_undo(gran_app* app, gran_undo_fn callback, void* user);

/* Живой документ: разметка остаётся после загрузки. markup_read — атрибут элемента узла target в разметке
 * (*result == NULL — атрибута нет или узел не из разметки; иначе освободить gran_string_free); markup_write
 * записывает атрибут (value == NULL — удаляет) и перестраивает узел: потомки строятся заново, их состояние
 * (текст полей, отметки, вкладки) переходит по id или месту; ошибка схемы — GRAN_ERROR (текст — gran_last_error), разметка прежняя.
 * markup_save — разметка окна узла window (NULL — главного) как XML с правками, вставками и удалениями.
 * The live document: the markup stays after loading. markup_read is an attribute of the target node element in the markup
 * (*result == NULL — no attribute or the node is not from the markup; otherwise free it with gran_string_free); markup_write
 * writes the attribute (value == NULL removes it) and rebuilds the node: the descendants are built anew, their state
 * (the field texts, check marks, tabs) moves by id or place; a schema error is GRAN_ERROR (the text is gran_last_error), the markup stays as it was.
 * markup_save is the markup of the window of the node window (NULL — the main one) as XML with the edits, insertions and removals. */
GRAN_API gran_status gran_app_markup_read(gran_app* app, const char* target, const char* name, char** result);
GRAN_API gran_status gran_app_markup_write(gran_app* app, const char* target, const char* name, const char* value);
GRAN_API gran_status gran_app_markup_save(gran_app* app, const char* window, char** result);

/* Окна: open_window открывает ещё одно окно (source — путь при is_file != 0, иначе разметка). options == NULL —
 * независимое окно; owner — id узла окна-владельца (подчинённое держится над ним и закрывается вместе с ним),
 * modal != 0 — владелец не получает ввода, пока окно открыто; on_closed(user, result) вызывается в потоке UI после
 * закрытия (result действителен только во время вызова; крестик и закрытие с владельцем — ""). close_window закрывает
 * окно узла target с результатом result (NULL — ""); закрытие главного окна завершает run.
 * Windows: open_window opens one more window (source is a path when is_file != 0, otherwise markup). options == NULL is
 * an independent window; owner is the id of a node of the owner window (the owned one stays above it and closes with it),
 * modal != 0 — the owner gets no input while the window is open; on_closed(user, result) is called on the UI thread after
 * closing (result is valid only during the call; the cross and closing with the owner give ""). close_window closes
 * the window of the target node with the result (NULL — ""); closing the main window ends run. */
typedef void (*gran_window_closed_fn)(void* user, const char* result);
typedef struct gran_window_options {
    uint32_t size;
    const char* owner;
    int32_t modal;
    gran_window_closed_fn on_closed;
    void* user;
} gran_window_options;
GRAN_API gran_status gran_app_open_window(gran_app* app, const char* source, int32_t is_file,
    const gran_window_options* options);
GRAN_API gran_status gran_app_close_window(gran_app* app, const char* target, const char* result);

/* Компоненты: сгенерированная XSD библиотеки namespace_uri, подключённой <xi:include> в окне узла window (NULL — главном);
 * пустая строка — такой библиотеки нет; освободить gran_string_free. Её сохраняют рядом с разметкой для подсказок редактора XML.
 * Components: the generated XSD of the namespace_uri library included with <xi:include> in the window of the node window (NULL — the main one);
 * an empty string — no such library; free it with gran_string_free. It is saved next to the markup for XML editor hints. */
GRAN_API gran_status gran_app_component_schema(gran_app* app, const char* namespace_uri, const char* window, char** result);

/* Выражения привязки {…}: переменные $имя и преобразователи «{выражение ; имя}». variable_set_text задаёт значение
 * с прежним типом переменной (новая — строка), _number и _boolean задают тип явно; variable_read — значение текстом
 * (*result == NULL — переменной нет; иначе освободить gran_string_free). converter регистрирует преобразователь:
 * to_view — значение выражения → текст свойства, to_source — обратно для двусторонней привязки (NULL — привязка
 * односторонняя); обратный вызов отвечает через gran_converted_set (без ответа значение остаётся входным), result
 * действителен только во время вызова; user должен жить, пока приложение существует. refresh_bindings — пересчитать
 * выражения сейчас, не дожидаясь кадра.
 * The {…} binding expressions: the $name variables and the "{expression ; name}" converters. variable_set_text sets a value
 * keeping the former variable type (a new one is a string), _number and _boolean set the type explicitly; variable_read is the value as text
 * (*result == NULL — no such variable; otherwise free it with gran_string_free). converter registers a converter:
 * to_view — the expression value → the property text, to_source — back for a two-way binding (NULL — the binding is
 * one-way); the callback answers through gran_converted_set (without an answer the value stays the input one), result
 * is valid only during the call; user must live while the application exists. refresh_bindings recomputes
 * the expressions now without waiting for a frame. */
typedef struct gran_converted gran_converted;
typedef void (*gran_convert_fn)(void* user, const char* value, gran_converted* result);
GRAN_API void gran_converted_set(gran_converted* result, const char* text);
GRAN_API gran_status gran_app_variable_set_text(gran_app* app, const char* name, const char* value);
GRAN_API gran_status gran_app_variable_set_number(gran_app* app, const char* name, double value);
GRAN_API gran_status gran_app_variable_set_boolean(gran_app* app, const char* name, int32_t value);
GRAN_API gran_status gran_app_variable_read(gran_app* app, const char* name, char** result);
GRAN_API gran_status gran_app_converter(gran_app* app, const char* name, gran_convert_fn to_view, gran_convert_fn to_source,
    void* user);
GRAN_API gran_status gran_app_refresh_bindings(gran_app* app);

/* Модели 1.7. Имена моделей общие с разметкой (<Data>, <Tree>, <SortFilter>, <TreeList>, <Selection>).
 * gran_model_count/read/set выше работают с любым списком (хранилище, прокси, плоское дерево; запись через
 * представление уходит в источник), insert/remove/clear/move/columns — только с хранилищем.
 * columns — описание столбцов таблицы (type — GRAN_COLUMN_*). Ленивая подгрузка из кода: on_fetch(model, fn) —
 * fn(user, model) вызывается, когда виртуальному списку нужна следующая порция (или по gran_model_fetch_more);
 * fn вставляет записи gran_model_insert и вызывает gran_model_set_more(app, model, 0), когда записей больше нет;
 * NULL снимает подгрузчик. load — список из файла JSON/XML (gran_source: fields — пары имя поля/путь, fetch > 0 —
 * лениво порциями), содержимое модели заменяется; ошибка файла — GRAN_ERROR с местом в gran_last_error.
 * Models 1.7. The model names are shared with the markup (<Data>, <Tree>, <SortFilter>, <TreeList>, <Selection>).
 * gran_model_count/read/set above work with any list (a storage, a proxy, a flat tree; a write through
 * a view goes into the source), insert/remove/clear/move/columns — with a storage only.
 * columns is the table column description (type is GRAN_COLUMN_*). Lazy loading from code: on_fetch(model, fn) —
 * fn(user, model) is called when a virtual list needs the next portion (or by gran_model_fetch_more);
 * fn inserts records with gran_model_insert and calls gran_model_set_more(app, model, 0) when there are no more records;
 * NULL drops the fetcher. load is a list from a JSON/XML file (gran_source: fields are field name/path pairs, fetch > 0 —
 * lazily by portions), the model contents are replaced; a file error is GRAN_ERROR with the place in gran_last_error. */
enum { GRAN_COLUMN_TEXT = 0, GRAN_COLUMN_NUMBER = 1, GRAN_COLUMN_DATE = 2, GRAN_COLUMN_BOOLEAN = 3 };
typedef struct gran_column {
    const char* name;
    const char* title;
    int32_t type;
} gran_column;
enum { GRAN_SOURCE_AUTO = 0, GRAN_SOURCE_JSON = 1, GRAN_SOURCE_XML = 2 };
typedef struct gran_source {
    uint32_t size;
    int32_t format;
    const char* path;
    const char* select;
    const char* children;
    size_t fetch;
    const gran_field* fields;
    size_t field_count;
} gran_source;
typedef void (*gran_fetch_fn)(void* user, const char* model);
GRAN_API gran_status gran_model_move(gran_app* app, const char* model, size_t from, size_t count, size_t to);
GRAN_API gran_status gran_model_columns(gran_app* app, const char* model, const gran_column* columns, size_t count);
GRAN_API gran_status gran_model_load(gran_app* app, const char* model, const gran_source* source);
GRAN_API gran_status gran_model_on_fetch(gran_app* app, const char* model, gran_fetch_fn callback, void* user);
GRAN_API gran_status gran_model_set_more(gran_app* app, const char* model, int32_t more);
GRAN_API gran_status gran_model_fetch_more(gran_app* app, const char* model);

/* Деревья: узлы — стабильные номера uint64 (0 — невидимый корень). insert — ребёнок parent на место record->index
 * (SIZE_MAX — в конец), *result — номер нового узла; remove(0) удаляет все узлы; move — к parent на место row (в новом
 * порядке); set меняет поля узла (record->index не используется); read — поле узла (освободить gran_string_free);
 * children — число детей, child — ребёнок по номеру, parent — родитель (у верхнего уровня — 0). set_lazy — «дети есть,
 * но не загружены»: при первом раскрытии вызывается fn(user, tree, node) из on_fetch, он вставляет детей. load — дерево
 * из файла (children — имя элемента XML или члена JSON с детьми).
 * Trees: the nodes are stable uint64 numbers (0 is the invisible root). insert is a child of parent at record->index
 * (SIZE_MAX — at the end), *result is the new node number; remove(0) removes all nodes; move is to parent at row (in the new
 * order); set changes the node fields (record->index is not used); read is a node field (free it with gran_string_free);
 * children is the child count, child is a child by row, parent is the parent (0 for the top level). set_lazy is "there are children,
 * but they are not loaded": on the first expansion fn(user, tree, node) from on_fetch is called, it inserts the children. load is a tree
 * from a file (children is the XML element or JSON member name with the children). */
typedef void (*gran_tree_fetch_fn)(void* user, const char* tree, uint64_t node);
GRAN_API gran_status gran_tree_insert(gran_app* app, const char* tree, uint64_t parent, const gran_record* record,
    uint64_t* result);
GRAN_API gran_status gran_tree_remove(gran_app* app, const char* tree, uint64_t node);
GRAN_API gran_status gran_tree_move(gran_app* app, const char* tree, uint64_t node, uint64_t parent, size_t row);
GRAN_API gran_status gran_tree_set(gran_app* app, const char* tree, uint64_t node, const gran_record* record);
GRAN_API gran_status gran_tree_read(gran_app* app, const char* tree, uint64_t node, const char* field, char** result);
GRAN_API gran_status gran_tree_children(gran_app* app, const char* tree, uint64_t node, size_t* result);
GRAN_API gran_status gran_tree_child(gran_app* app, const char* tree, uint64_t node, size_t row, uint64_t* result);
GRAN_API gran_status gran_tree_parent(gran_app* app, const char* tree, uint64_t node, uint64_t* result);
GRAN_API gran_status gran_tree_set_lazy(gran_app* app, const char* tree, uint64_t node, int32_t lazy);
GRAN_API gran_status gran_tree_on_fetch(gran_app* app, const char* tree, gran_tree_fetch_fn callback, void* user);
GRAN_API gran_status gran_tree_load(gran_app* app, const char* tree, const gran_source* source);

/* Представления. view_create создаёт прокси (GRAN_VIEW_SORT_FILTER над списком source), плоское дерево
 * (GRAN_VIEW_TREE_LIST над деревом source, expanded уровней раскрыто) или выделение (GRAN_VIEW_SELECTION над списком
 * source, mode — GRAN_SELECT_*); sort и filter могут быть NULL. view_sort — ключи «поле[:text|number|natural] [asc|desc]»
 * через запятую; view_filter — XPath 1.0 по записи с $переменными (NULL или "" — снять); source_index — номер записи
 * источника для строки прокси; tree_row — узел и признаки строки плоского дерева; tree_expand — action 1 раскрыть,
 * 0 свернуть, -1 переключить.
 * Views. view_create creates a proxy (GRAN_VIEW_SORT_FILTER over the source list), a flat tree
 * (GRAN_VIEW_TREE_LIST over the source tree, expanded levels are expanded) or a selection (GRAN_VIEW_SELECTION over the source
 * list, mode is GRAN_SELECT_*); sort and filter may be NULL. view_sort is the comma separated "field[:text|number|natural] [asc|desc]"
 * keys; view_filter is XPath 1.0 over the record with $variables (NULL or "" — drop); source_index is the source record index
 * for a proxy row; tree_row is the node and the flags of a flat tree row; tree_expand — action 1 expand,
 * 0 collapse, -1 toggle. */
enum { GRAN_VIEW_SORT_FILTER = 1, GRAN_VIEW_TREE_LIST = 2, GRAN_VIEW_SELECTION = 3 };
enum { GRAN_SELECT_NONE = 0, GRAN_SELECT_SINGLE = 1, GRAN_SELECT_MULTI = 2, GRAN_SELECT_EXTENDED = 3 };
typedef struct gran_view {
    uint32_t size;
    int32_t kind;
    const char* name;
    const char* source;
    const char* sort;
    const char* filter;
    int32_t mode;
    size_t expanded;
} gran_view;
typedef struct gran_tree_row {
    uint64_t node;
    size_t depth;
    int32_t expanded;
    int32_t has_children;
} gran_tree_row;
GRAN_API gran_status gran_view_create(gran_app* app, const gran_view* view);
GRAN_API gran_status gran_view_sort(gran_app* app, const char* view, const char* keys);
GRAN_API gran_status gran_view_filter(gran_app* app, const char* view, const char* expression);
GRAN_API gran_status gran_view_source_index(gran_app* app, const char* view, size_t row, size_t* result);
GRAN_API gran_status gran_tree_row_read(gran_app* app, const char* view, size_t row, gran_tree_row* result);
GRAN_API gran_status gran_tree_expand(gran_app* app, const char* view, size_t row, int32_t action);

/* Выделение: change — GRAN_SELECTION_* с записью index (RANGE — от index до last включительно; CURRENT — сделать
 * текущей, SIZE_MAX — снять; CLICK — как щелчок: last — модификаторы 1 Shift, 2 Ctrl). read — текущая запись
 * (*current == SIZE_MAX — нет) и отмеченные: indices — массив на *count мест (может быть NULL), *count на выходе —
 * число отмеченных. on_change — fn(user, selection) при любом изменении (NULL снимает).
 * Selection: change is GRAN_SELECTION_* with the record index (RANGE — from index to last inclusive; CURRENT — make it
 * current, SIZE_MAX — drop; CLICK — as a click: last is the modifiers 1 Shift, 2 Ctrl). read is the current record
 * (*current == SIZE_MAX — none) and the marked ones: indices is an array of *count places (may be NULL), *count on output is
 * the marked count. on_change is fn(user, selection) on any change (NULL drops it). */
enum {
    GRAN_SELECTION_SELECT = 1, GRAN_SELECTION_DESELECT = 2, GRAN_SELECTION_TOGGLE = 3, GRAN_SELECTION_RANGE = 4,
    GRAN_SELECTION_CLEAR = 5, GRAN_SELECTION_ALL = 6, GRAN_SELECTION_CURRENT = 7, GRAN_SELECTION_CLICK = 8
};
typedef void (*gran_selection_fn)(void* user, const char* selection);
GRAN_API gran_status gran_selection_change(gran_app* app, const char* selection, int32_t command, size_t index,
    size_t last);
GRAN_API gran_status gran_selection_read(gran_app* app, const char* selection, size_t* current, size_t* indices,
    size_t* count);
GRAN_API gran_status gran_selection_on_change(gran_app* app, const char* selection, gran_selection_fn callback,
    void* user);

#ifdef __cplusplus
}
#endif
#endif
