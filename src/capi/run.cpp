#include "gran/capi/internal/bridge.hpp"

namespace
{
    // Сбрасываем флаг также при исключении графики или пользовательского callback C++.
    // Reset the flag on graphics errors and exceptions from a C++ user callback too.
    struct RunScope
    {
        gran_app& app;
        explicit RunScope(gran_app& owner);
        ~RunScope();
    };
    RunScope::RunScope(gran_app& owner) : app(owner) { app.running = true; }
    RunScope::~RunScope() { app.running = false; }
}

// Повторный вход блокируется до вызова Application::run.
// Re-entry is blocked before calling Application::run.
gran_status gran_app_run(gran_app* app, int32_t* exitCode)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(exitCode != nullptr);
    if (app->running)
        throw gran::capi::Failure{GRAN_BUSY, gran::i18n::format(gran::i18n::Message::CAPI_BUSY)};
    RunScope scope(*app);
    *exitCode = app->application.run();
    GRAN_CAPI_END
}
