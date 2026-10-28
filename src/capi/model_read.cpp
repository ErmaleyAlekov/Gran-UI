#include "gran/capi/internal/bridge.hpp"

// Именованные модели живут в ядре и сохраняются при reload.
// Named models live in the core and survive reload.
gran_status gran_model_count(gran_app* app, const char* model, size_t* result)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(model != nullptr && result != nullptr);
    *result = app->application.model(model).size();
    GRAN_CAPI_END
}

// Отсутствующее поле — пустая строка; несуществующая запись — ошибка ядра.
// A missing field is an empty string; a missing record is a core error.
gran_status gran_model_read(gran_app* app, const char* model, size_t index, const char* field, char** result)
{
    if (result != nullptr) *result = nullptr;
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(model != nullptr && field != nullptr && result != nullptr);
    const gran::data::Record& row = app->application.model(model).at(index);
    auto found = row.find(field);
    std::string value;
    if (found != row.end()) value = found->second;
    *result = gran::capi::copyString(value);
    GRAN_CAPI_END
}
