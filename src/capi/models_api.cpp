#include "gran/capi/internal/bridge.hpp"

namespace gran::capi
{
    // Размер структуры защищает ABI: поля добавляются только в конец / The structure size protects the ABI: fields are only appended
    data::SourceSpec sourceSpec(const gran_source* source)
    {
        data::SourceSpec spec;

        require(source != nullptr && source->size >= sizeof(gran_source) && source->path != nullptr);
        require(source->field_count == 0 || source->fields != nullptr);
        spec.path = source->path;
        spec.select = text(source->select);
        spec.children = text(source->children);
        spec.fetch = source->fetch;
        if (source->format == GRAN_SOURCE_JSON)
            spec.format = data::SourceFormat::JSON;
        else if (source->format == GRAN_SOURCE_XML)
            spec.format = data::SourceFormat::XML;
        for (size_t index = 0; index < source->field_count; ++index)
        {
            const gran_field& field = source->fields[index];
            require(field.name != nullptr && field.value != nullptr);
            spec.fields.push_back(data::FieldSpec{field.name, field.value});
        }
        return spec;
    }
}

gran_status gran_model_move(gran_app* app, const char* model, size_t from, size_t count, size_t to)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(model != nullptr);
    app->application.model(model).move(from, count, to);
    GRAN_CAPI_END
}

// Тип вне перечисления — ошибка аргумента, а не молчаливый текст / A type outside the enumeration is an argument error, not a silent text
gran_status gran_model_columns(gran_app* app, const char* model, const gran_column* columns, size_t count)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(model != nullptr && (count == 0 || columns != nullptr));
    gran::data::ColumnSet set;
    for (size_t index = 0; index < count; ++index)
    {
        gran::capi::require(columns[index].name != nullptr && columns[index].type >= 0 && columns[index].type <= 3);
        set.push_back(gran::data::Column{columns[index].name, gran::capi::text(columns[index].title),
            static_cast<gran::data::ColumnType>(columns[index].type)});
    }
    app->application.model(model).setColumns(std::move(set));
    GRAN_CAPI_END
}

gran_status gran_model_load(gran_app* app, const char* model, const gran_source* source)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(model != nullptr);
    app->application.loadData(model, gran::capi::sourceSpec(source));
    GRAN_CAPI_END
}

gran_status gran_tree_load(gran_app* app, const char* tree, const gran_source* source)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(tree != nullptr);
    app->application.loadTree(tree, gran::capi::sourceSpec(source));
    GRAN_CAPI_END
}
