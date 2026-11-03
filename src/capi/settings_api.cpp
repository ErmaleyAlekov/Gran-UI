#include "gran/capi/internal/bridge.hpp"

#include <cstdlib>

// Путь или файл по имени программы; повторная загрузка заменяет настройки / A path or the file by the program name; a repeated load replaces the settings
gran_status gran_settings_load(gran_app* app, const char* app_name, const char* path)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(path != nullptr || (app_name != nullptr && app_name[0] != '\0'));
    if (path != nullptr)
        app->application.settings().load(path);
    else
        app->application.settings().load(gran::settings::defaultPath(app_name));
    GRAN_CAPI_END
}

// Тип и текст значения; у списка текста нет / The value type and text; a list has no text
gran_status gran_settings_get(gran_app* app, const char* name, int32_t* type, char** result)
{
    if (type != nullptr) *type = GRAN_SETTING_MISSING;
    if (result != nullptr) *result = nullptr;
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(name != nullptr && type != nullptr && result != nullptr);
    const gran::settings::Value* value = app->application.settings().find(name);
    if (value == nullptr)
        return GRAN_OK;
    *type = static_cast<int32_t>(value->type);
    if (value->type != gran::settings::ValueType::LIST)
        *result = gran::capi::copyString(value->text);
    GRAN_CAPI_END
}

// Число — strtod по всей строке, логическое — "true"/"false"; иное — неверный аргумент / A number — strtod over the whole string, a boolean — "true"/"false"; otherwise an invalid argument
gran_status gran_settings_set(gran_app* app, const char* name, int32_t type, const char* value)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(name != nullptr && value != nullptr);
    gran::capi::require(type >= GRAN_SETTING_STRING && type <= GRAN_SETTING_BOOLEAN);
    gran::settings::Settings& settings = app->application.settings();
    char* end = nullptr;
    if (type == GRAN_SETTING_STRING)
        settings.setString(name, value);
    else if (type == GRAN_SETTING_NUMBER)
    {
        double number = std::strtod(value, &end);
        gran::capi::require(end != value && *end == '\0');
        settings.setNumber(name, number);
    }
    else
    {
        gran::capi::require(std::string(value) == "true" || std::string(value) == "false");
        settings.setBool(name, std::string(value) == "true");
    }
    GRAN_CAPI_END
}
