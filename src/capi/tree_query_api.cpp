#include "gran/capi/internal/bridge.hpp"

gran_status gran_tree_children(gran_app* app, const char* tree, uint64_t node, size_t* result)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(tree != nullptr && result != nullptr);
    gran::data::TreeModel& model = app->application.tree(tree);
    *result = model.childCount(gran::capi::treeNode(model, node));
    GRAN_CAPI_END
}

gran_status gran_tree_child(gran_app* app, const char* tree, uint64_t node, size_t row, uint64_t* result)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(tree != nullptr && result != nullptr);
    gran::data::TreeModel& model = app->application.tree(tree);
    *result = model.child(gran::capi::treeNode(model, node), row);
    GRAN_CAPI_END
}

// У узла верхнего уровня родитель — корень, в ABI это 0 / A top level node parent is the root, which is 0 in the ABI
gran_status gran_tree_parent(gran_app* app, const char* tree, uint64_t node, uint64_t* result)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(tree != nullptr && result != nullptr && node != 0);
    gran::data::TreeModel& model = app->application.tree(tree);
    *result = gran::capi::abiNode(model, model.parent(node));
    GRAN_CAPI_END
}

gran_status gran_tree_set_lazy(gran_app* app, const char* tree, uint64_t node, int32_t lazy)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(tree != nullptr && node != 0);
    app->application.tree(tree).setLazy(node, lazy != 0);
    GRAN_CAPI_END
}
