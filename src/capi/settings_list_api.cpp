#include "gran/capi/internal/bridge.hpp"

#include <string>
#include <vector>

// Элемент списка по номеру; за концом — NULL без ошибки / A list item by number; past the end — NULL without an error
gran_status gran_settings_list_item(gran_app* app, const char* name, size_t index, char** result)
{
    if (result != nullptr) *result = nullptr;
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(name != nullptr && result != nullptr);
    const gran::settings::Value* value = app->application.settings().find(name);
    if (value != nullptr && value->type == gran::settings::ValueType::LIST && index < value->items.size())
        *result = gran::capi::copyString(value->items[index]);
    GRAN_CAPI_END
}

// Список строк заменяет прежнее значение / A list of strings replaces the former value
gran_status gran_settings_set_list(gran_app* app, const char* name, const char* const* items, size_t count)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(name != nullptr && (items != nullptr || count == 0));
    std::vector<std::string> list;
    for (size_t index = 0; index < count; ++index)
        list.push_back(gran::capi::text(items[index]));
    app->application.settings().setList(name, list);
    GRAN_CAPI_END
}

// Удалить значение; не было — не ошибка / Remove a value; it did not exist — not an error
gran_status gran_settings_remove(gran_app* app, const char* name)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(name != nullptr);
    app->application.settings().remove(name);
    GRAN_CAPI_END
}

// Сохранить сейчас; без пути (настройки не загружены) — GRAN_ERROR / Save now; without a path (the settings are not loaded) — GRAN_ERROR
gran_status gran_settings_save(gran_app* app)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    if (!app->application.settings().save())
        throw gran::GranException(gran::ErrorCode::IO, gran::i18n::format(gran::i18n::Message::IO_WRITE_FAILED,
            {app->application.settings().path()}));
    GRAN_CAPI_END
}
