#include "gran/capi/internal/bridge.hpp"

#include <optional>

// Нет переменной — NULL, а не пустая строка: пустое значение тоже бывает / No variable is NULL, not an empty string: an empty value exists too
gran_status gran_app_variable_read(gran_app* app, const char* name, char** result)
{
    if (result != nullptr) *result = nullptr;
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(name != nullptr && result != nullptr);
    std::optional<std::string> value = app->application.variable(name);
    if (value)
        *result = gran::capi::copyString(*value);
    GRAN_CAPI_END
}

// Пустой to_view — значение как есть; пустой to_source — привязка односторонняя / A null to_view is the value as it is; a null to_source makes the binding one-way
gran_status gran_app_converter(gran_app* app, const char* name, gran_convert_fn to_view, gran_convert_fn to_source, void* user)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(name != nullptr && *name != '\0');
    gran::binding::Converter converter;
    if (to_view != nullptr)
        converter.toView = gran::capi::ConvertCallback{to_view, user};
    if (to_source != nullptr)
        converter.toSource = gran::capi::ConvertCallback{to_source, user};
    app->application.setConverter(name, std::move(converter));
    GRAN_CAPI_END
}

gran_status gran_app_refresh_bindings(gran_app* app)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    app->application.refreshBindings();
    GRAN_CAPI_END
}
