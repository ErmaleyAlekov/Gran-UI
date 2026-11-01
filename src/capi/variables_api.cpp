#include "gran/capi/internal/bridge.hpp"

namespace gran::capi
{
    // Ответ живёт в стеке вызова: языковая сторона копирует строку в него через gran_converted_set / The answer lives on the call stack: the language side copies a string into it through gran_converted_set
    std::string ConvertCallback::operator()(const std::string& value) const
    {
        gran_converted result{value};

        callback(user, value.c_str(), &result);
        return result.text;
    }
}

// NULL — пустая строка (как у прочих текстов ABI) / NULL is an empty string (as for the other ABI texts)
void gran_converted_set(gran_converted* result, const char* text)
{
    if (result != nullptr)
        result->text = gran::capi::text(text);
}

gran_status gran_app_variable_set_text(gran_app* app, const char* name, const char* value)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(name != nullptr && *name != '\0');
    app->application.setVariable(name, gran::capi::text(value));
    GRAN_CAPI_END
}

gran_status gran_app_variable_set_number(gran_app* app, const char* name, double value)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(name != nullptr && *name != '\0');
    app->application.setVariableNumber(name, value);
    GRAN_CAPI_END
}

gran_status gran_app_variable_set_boolean(gran_app* app, const char* name, int32_t value)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(name != nullptr && *name != '\0');
    app->application.setVariableBoolean(name, value != 0);
    GRAN_CAPI_END
}
