#include "gran/capi/internal/bridge.hpp"

// Подгрузчик запоминается в дескрипторе: set_more находит его по имени модели / The fetcher is remembered in the handle: set_more finds it by the model name
gran_status gran_model_on_fetch(gran_app* app, const char* model, gran_fetch_fn callback, void* user)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(model != nullptr);
    gran::data::ListModel& list = app->application.model(model);
    if (callback == nullptr)
    {
        app->fetchers.erase(model);
        list.setFetcher(nullptr);
        return GRAN_OK;
    }
    auto fetcher = std::make_shared<gran::capi::ListCallbackFetcher>(callback, user, model);
    app->fetchers[model] = fetcher;
    list.setFetcher(fetcher);
    GRAN_CAPI_END
}

// «Есть ещё» после clear возвращает подгрузчик модели (clear его снимает) / "There is more" after clear gives the fetcher back to the model (clear drops it)
gran_status gran_model_set_more(gran_app* app, const char* model, int32_t more)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(model != nullptr);
    auto found = app->fetchers.find(model);
    gran::capi::require(found != app->fetchers.end());
    found->second->more = more != 0;
    if (more != 0)
        app->application.model(model).setFetcher(found->second);
    GRAN_CAPI_END
}

gran_status gran_model_fetch_more(gran_app* app, const char* model)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(model != nullptr);
    app->application.list(model).fetchMore();
    GRAN_CAPI_END
}

gran_status gran_tree_on_fetch(gran_app* app, const char* tree, gran_tree_fetch_fn callback, void* user)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(tree != nullptr);
    std::shared_ptr<gran::data::TreeFetcher> fetcher;
    if (callback != nullptr)
        fetcher = std::make_shared<gran::capi::TreeCallbackFetcher>(callback, user, tree);
    app->application.tree(tree).setFetcher(fetcher);
    GRAN_CAPI_END
}
