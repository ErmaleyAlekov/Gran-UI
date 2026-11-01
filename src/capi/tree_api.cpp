#include "gran/capi/internal/bridge.hpp"

// Место SIZE_MAX — в конец, как у gran_model_insert / The place SIZE_MAX — at the end, as for gran_model_insert
gran_status gran_tree_insert(gran_app* app, const char* tree, uint64_t parent, const gran_record* record, uint64_t* result)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(tree != nullptr && result != nullptr);
    gran::data::Record fields = gran::capi::record(record);
    gran::data::TreeModel& model = app->application.tree(tree);
    gran::data::NodeId owner = gran::capi::treeNode(model, parent);
    size_t row = record->index;
    if (row == SIZE_MAX)
        row = model.childCount(owner);
    *result = model.insert(owner, row, std::move(fields));
    GRAN_CAPI_END
}

// Корень не удаляется: remove(0) — удалить все узлы / The root is not removed: remove(0) removes all nodes
gran_status gran_tree_remove(gran_app* app, const char* tree, uint64_t node)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(tree != nullptr);
    gran::data::TreeModel& model = app->application.tree(tree);
    if (node == 0)
        model.clear();
    else
        model.remove(node);
    GRAN_CAPI_END
}

gran_status gran_tree_move(gran_app* app, const char* tree, uint64_t node, uint64_t parent, size_t row)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(tree != nullptr && node != 0);
    gran::data::TreeModel& model = app->application.tree(tree);
    model.move(node, gran::capi::treeNode(model, parent), row);
    GRAN_CAPI_END
}

gran_status gran_tree_set(gran_app* app, const char* tree, uint64_t node, const gran_record* record)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(tree != nullptr && node != 0);
    gran::data::Record fields = gran::capi::record(record);
    gran::data::TreeModel& model = app->application.tree(tree);
    model.record(node);
    for (const auto& field : fields)
        model.set(node, field.first, field.second);
    GRAN_CAPI_END
}

gran_status gran_tree_read(gran_app* app, const char* tree, uint64_t node, const char* field, char** result)
{
    if (result != nullptr) *result = nullptr;
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(tree != nullptr && field != nullptr && result != nullptr && node != 0);
    const gran::data::Record& record = app->application.tree(tree).record(node);
    auto found = record.find(field);
    std::string value;
    if (found != record.end()) value = found->second;
    *result = gran::capi::copyString(value);
    GRAN_CAPI_END
}
