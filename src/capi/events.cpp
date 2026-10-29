#include "gran/capi/internal/bridge.hpp"

// Пустой callback снимает обработчик: имя снова ищется среди on_action.
// A null callback clears the handler: the name is looked for among on_action again.
gran_status gran_app_on_event(gran_app* app, const char* name, gran_event_fn callback, void* user)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(name != nullptr && name[0] != '\0');
    if (callback == nullptr) app->application.onEvent(name, gran::EventHandler());
    else app->application.onEvent(name, gran::capi::EventCallback{app, callback, user});
    GRAN_CAPI_END
}

// Узел ищется по id сейчас; слушатель живёт в узле и уходит вместе с ним.
// The node is found by id now; the listener lives in the node and goes away together with it.
gran_status gran_app_listen(gran_app* app, const char* target, const char* type, int32_t capture,
    gran_event_fn callback, void* user, uint64_t* result)
{
    if (result != nullptr) *result = 0;
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(type != nullptr && type[0] != '\0' && callback != nullptr && result != nullptr);
    gran::ui::Node& node = gran::app::requireNode(app->application, gran::capi::text(target));
    *result = app->application.addEventListener(node, type, gran::capi::EventCallback{app, callback, user}, capture != 0);
    GRAN_CAPI_END
}

gran_status gran_app_unlisten(gran_app* app, const char* target, uint64_t listener)
{
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::ui::Node& node = gran::app::requireNode(app->application, gran::capi::text(target));
    app->application.removeEventListener(node, listener);
    GRAN_CAPI_END
}

// allowed == NULL — результат не нужен вызывающему.
// allowed == NULL — the caller does not need the result.
gran_status gran_app_dispatch(gran_app* app, const char* target, const char* type, const char* detail,
    int32_t* allowed)
{
    if (allowed != nullptr) *allowed = 1;
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(type != nullptr && type[0] != '\0');
    gran::ui::Node& node = gran::app::requireNode(app->application, gran::capi::text(target));
    bool result = app->application.dispatchEvent(node, type, gran::capi::text(detail));
    if (allowed != nullptr && !result) *allowed = 0;
    GRAN_CAPI_END
}
