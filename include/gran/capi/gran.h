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
 * showPopup/hidePopup/navigate/loadHtml/postMessage/appendMarkup/capturePointer/releasePointer.
 * target и argument могут быть NULL.
 * command: close/minimize/maximize/restore/reload/focus/remove/showDialog/hideDialog/
 * showPopup/hidePopup/navigate/loadHtml/postMessage/appendMarkup/capturePointer/releasePointer.
 * target and argument may be NULL. */
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

#ifdef __cplusplus
}
#endif
#endif
