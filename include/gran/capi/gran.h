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
 * showPopup/hidePopup/navigate/loadHtml/postMessage/appendMarkup. target и argument могут быть NULL.
 * command: close/minimize/maximize/restore/reload/focus/remove/showDialog/hideDialog/
 * showPopup/hidePopup/navigate/loadHtml/postMessage/appendMarkup. target and argument may be NULL. */
GRAN_API gran_status gran_app_command(gran_app* app, const char* command, const char* target, const char* argument);
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
