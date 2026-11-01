#include "gran/capi/internal/bridge.hpp"

// Вид и режим проверяются до создания: неверный аргумент не оставляет полусозданного представления / The kind and the mode are checked before creating: a wrong argument leaves no half-created view
gran_status gran_view_create(gran_app* app, const gran_view* view)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(view != nullptr && view->size >= sizeof(gran_view) && view->name != nullptr && view->source != nullptr);
    gran::capi::require(view->kind >= GRAN_VIEW_SORT_FILTER && view->kind <= GRAN_VIEW_SELECTION);
    gran::capi::require(view->mode >= GRAN_SELECT_NONE && view->mode <= GRAN_SELECT_EXTENDED);
    gran::Application& application = app->application;
    if (view->kind == GRAN_VIEW_SELECTION)
    {
        application.createSelection(view->name, view->source, static_cast<gran::data::SelectionMode>(view->mode));
        return GRAN_OK;
    }
    gran::data::SortKeys keys = gran::data::parseSortKeys(gran::capi::text(view->sort));
    if (view->kind == GRAN_VIEW_SORT_FILTER)
        application.createProxy(view->name, view->source).setSort(keys);
    else
        application.createTreeList(view->name, view->source).setSort(keys);
    application.setFilterExpression(view->name, gran::capi::text(view->filter));
    if (view->kind == GRAN_VIEW_TREE_LIST)
        application.treeList(view->name).expandToDepth(view->expanded);
    GRAN_CAPI_END
}

gran_status gran_view_sort(gran_app* app, const char* view, const char* keys)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(view != nullptr);
    gran::data::SortKeys parsed = gran::data::parseSortKeys(gran::capi::text(keys));
    gran::data::TreeList* rows = dynamic_cast<gran::data::TreeList*>(&app->application.list(view));
    if (rows != nullptr)
        rows->setSort(std::move(parsed));
    else
        app->application.proxy(view).setSort(std::move(parsed));
    GRAN_CAPI_END
}

gran_status gran_view_filter(gran_app* app, const char* view, const char* expression)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(view != nullptr);
    app->application.setFilterExpression(view, gran::capi::text(expression));
    GRAN_CAPI_END
}

gran_status gran_view_source_index(gran_app* app, const char* view, size_t row, size_t* result)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(view != nullptr && result != nullptr);
    *result = app->application.proxy(view).sourceIndex(row);
    GRAN_CAPI_END
}

gran_status gran_tree_row_read(gran_app* app, const char* view, size_t row, gran_tree_row* result)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(view != nullptr && result != nullptr);
    gran::data::TreeList& rows = app->application.treeList(view);
    result->node = gran::capi::abiNode(rows.tree(), rows.node(row));
    result->depth = rows.depth(row);
    result->expanded = static_cast<int32_t>(rows.expanded(row));
    result->has_children = static_cast<int32_t>(rows.hasChildren(row));
    GRAN_CAPI_END
}
