#include "gran/capi/internal/bridge.hpp"
#include "gran/app/commands.hpp"

namespace
{
    // Обработчик закрытия окна для языковой стороны: только callback и user / A window close handler for the language side: only callback and user
    struct WindowClosed
    {
        gran_window_closed_fn callback;
        void* user;

        // Обратный вызов не бросает исключений (правило C ABI) / The callback throws no exceptions (the C ABI rule)
        void operator()(const std::string& result) const
        {
            callback(user, result.c_str());
        }
    };
}

// Владелец — по id узла; size защищает от старой структуры меньшего размера / The owner is by the node id; size protects against an older smaller structure
gran_status gran_app_open_window(gran_app* app, const char* source, int32_t is_file, const gran_window_options* options)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(source != nullptr);
    gran::WindowOptions window;
    if (options != nullptr)
    {
        gran::capi::require(options->size >= sizeof(gran_window_options));
        if (options->owner != nullptr)
            window.owner = &gran::app::requireNode(app->application, options->owner);
        window.modal = options->modal != 0;
        if (options->on_closed != nullptr)
            window.onClosed = WindowClosed{options->on_closed, options->user};
    }
    if (is_file != 0)
        app->application.openWindow(source, window);
    else
        app->application.openWindowString(source, window);
    GRAN_CAPI_END
}

// Результат NULL — пустая строка, как у крестика / A NULL result is an empty string, as for the cross
gran_status gran_app_close_window(gran_app* app, const char* target, const char* result)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(target != nullptr);
    app->application.closeWindow(gran::app::requireNode(app->application, target), gran::capi::text(result));
    GRAN_CAPI_END
}
