#include "gran/capi/internal/bridge.hpp"

namespace gran::capi
{
    // Сначала проверяем и копируем все поля, затем уведомляем привязки.
    // Validate and copy all fields before notifying bindings.
    data::Record record(const gran_record* value)
    {
        require(value != nullptr);
        require(value->count == 0 || value->fields != nullptr);
        data::Record result;
        for (size_t index = 0; index < value->count; ++index)
        {
            const gran_field& field = value->fields[index];
            require(field.name != nullptr && field.value != nullptr && field.name[0] != '\0');
            result[field.name] = field.value;
        }
        return result;
    }
}

gran_status gran_model_insert(gran_app* app, const char* model, const gran_record* record)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(model != nullptr);
    gran::data::Record fields = gran::capi::record(record);
    gran::data::ListModel& list = app->application.model(model);
    size_t index = record->index;
    if (index == SIZE_MAX) index = list.size();
    list.insert(index, std::move(fields));
    GRAN_CAPI_END
}

gran_status gran_model_set(gran_app* app, const char* model, const gran_record* record)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(model != nullptr);
    gran::data::Record fields = gran::capi::record(record);
    gran::data::ListSource& list = app->application.list(model);
    list.at(record->index);
    for (const auto& field : fields)
        list.set(record->index, field.first, field.second);
    GRAN_CAPI_END
}

gran_status gran_model_remove(gran_app* app, const char* model, size_t index)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(model != nullptr);
    app->application.model(model).remove(index);
    GRAN_CAPI_END
}

gran_status gran_model_clear(gran_app* app, const char* model)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(model != nullptr);
    app->application.model(model).clear();
    GRAN_CAPI_END
}
