#include "gran/capi/internal/bridge.hpp"
#include <cmath>

// Идентификатор создавшего потока фиксируется один раз.
// The creating thread identifier is captured once.
gran_app::gran_app(gran::AppOptions options) : application(options), thread(std::this_thread::get_id()) {}

namespace
{
    // Размер структуры защищает ABI от несовместимых версий; нулевой указатель — умолчания.
    // Structure size protects the ABI from incompatible versions; a null pointer selects defaults.
    gran::AppOptions readOptions(const gran_options* source)
    {
        gran::AppOptions result;
        if (source == nullptr) return result;
        gran::capi::require(source->size == sizeof(gran_options));
        gran::capi::require(source->backend >= 0 && source->backend <= 3);
        gran::capi::require(std::isfinite(source->exit_after) && source->exit_after >= 0.0);
        result.backend = static_cast<gran::rhi::BackendKind>(source->backend);
        result.debugLayer = source->debug_layer != 0;
        result.forceWarp = source->force_warp != 0;
        result.watchMarkup = source->watch_markup != 0;
        result.vsync = source->no_vsync == 0;
        result.exitAfter = source->exit_after;
        return result;
    }
}

// Ошибка создания всегда оставляет result пустым.
// A creation error always leaves result empty.
gran_status gran_app_create(const gran_options* options, gran_app** result)
{
    if (result != nullptr) *result = nullptr;
    GRAN_CAPI_TRY
    gran::capi::require(result != nullptr);
    *result = new gran_app(readOptions(options));
    GRAN_CAPI_END
}

// Запрет разрушения из callback предотвращает возврат в освобождённый цикл событий.
// Blocking destruction inside callbacks prevents returning to a freed event loop.
gran_status gran_app_destroy(gran_app* app)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    if (app->running)
        throw gran::capi::Failure{GRAN_BUSY, gran::i18n::format(gran::i18n::Message::CAPI_BUSY)};
    delete app;
    GRAN_CAPI_END
}

// Загрузка и проверка XML выполняются только ядром.
// XML loading and validation run only in the core.
gran_status gran_app_load(gran_app* app, const char* source, int32_t isFile)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(source != nullptr);
    if (isFile != 0) app->application.loadFile(source);
    else app->application.loadString(source);
    GRAN_CAPI_END
}
