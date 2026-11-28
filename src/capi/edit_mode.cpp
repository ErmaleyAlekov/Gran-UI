#include "gran/capi/internal/bridge.hpp"

// Режим меняется в потоке приложения через общий C++ интерфейс.
// Change the mode on the application thread through the common C++ interface.
gran_status gran_app_edit_mode(gran_app* app, int32_t enabled)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    app->application.setEditMode(enabled != 0);
    GRAN_CAPI_END
}

// Во время обратного вызова запрещаем разрушение приложения через C ABI.
// Block destruction of the application through the C ABI during the callback.
gran_status gran_app_on_layout_changed(gran_app* app, gran_task_fn callback, void* user)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::Task handler;
    if (callback)
        handler = [app, callback, user] {
            const bool previous = app->running;
            app->running = true;
            try { callback(user); }
            catch (...) { app->running = previous; throw; }
            app->running = previous;
        };
    app->application.onLayoutChanged(std::move(handler));
    GRAN_CAPI_END
}
