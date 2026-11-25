#include "gran/capi/internal/bridge.hpp"

namespace
{
    // Копии в очереди доставки разделяют владение контекстом. / Dispatch snapshots share ownership of the context.
    struct OwnedListener
    {
        gran::capi::EventCallback event{};
        gran_task_fn release = nullptr;
        ~OwnedListener()
        {
            if (release == nullptr)
                return;
            event.app->releasingListener = true;
            release(event.user);
            event.app->releasingListener = false;
        }
    };
}

gran_status gran_app_listen_owned(gran_app* app, const char* target, const char* type,
    const gran_listener* listener, uint64_t* result)
{
    if (result != nullptr) *result = 0;
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(type != nullptr && *type != '\0' && result != nullptr && listener != nullptr
        && listener->size == sizeof(gran_listener) && listener->callback != nullptr && listener->release != nullptr);
    auto& node = gran::app::requireNode(app->application, gran::capi::text(target));
    auto owner = std::make_shared<OwnedListener>();
    owner->event = {app, listener->callback, listener->user};
    *result = app->application.addEventListener(node, type,
        [owner](gran::ui::Node& current, gran::ui::UiEvent& event) { owner->event(current, event); }, listener->capture != 0);
    owner->release = listener->release;
    GRAN_CAPI_END
}
