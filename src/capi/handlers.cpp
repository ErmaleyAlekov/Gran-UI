#include "gran/capi/internal/bridge.hpp"
#include <cmath>

// Пустой callback снимает именованный обработчик.
// A null callback clears a named handler.
gran_status gran_app_on_action(gran_app* app, const char* name, gran_action_fn callback, void* user)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(name != nullptr && name[0] != '\0');
    app->application.onAction(name, gran::capi::Action{app, callback, user});
    GRAN_CAPI_END
}

// Сообщения WebView используют тот же безопасный снимок события.
// WebView messages use the same safe event snapshot.
gran_status gran_app_on_message(gran_app* app, const char* name, gran_action_fn callback, void* user)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(name != nullptr && name[0] != '\0');
    app->application.onWebMessage(name, gran::capi::Action{app, callback, user});
    GRAN_CAPI_END
}

// Единственный вызов из фонового потока; callback исполняет цикл UI.
// The only call from a background thread; the UI loop runs the callback.
gran_status gran_app_post(gran_app* app, gran_task_fn callback, void* user)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app, false);
    gran::capi::require(callback != nullptr);
    app->application.post(gran::capi::Task{callback, user});
    GRAN_CAPI_END
}

// Сроки NaN/Infinity отклоняются до попадания в очередь таймеров.
// NaN/Infinity deadlines are rejected before reaching the timer queue.
gran_status gran_app_timer(gran_app* app, const gran_timer* timer, uint64_t* result)
{
    if (result != nullptr) *result = 0;
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(timer != nullptr && result != nullptr && timer->callback != nullptr);
    gran::capi::require(std::isfinite(timer->seconds) && timer->seconds >= 0.0);
    gran::capi::Task callback{timer->callback, timer->user};
    if (timer->repeat != 0) *result = app->application.setInterval(timer->seconds, callback);
    else *result = app->application.setTimeout(timer->seconds, callback);
    GRAN_CAPI_END
}

gran_status gran_app_clear_timer(gran_app* app, uint64_t timer)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    app->application.clearTimer(timer);
    GRAN_CAPI_END
}
