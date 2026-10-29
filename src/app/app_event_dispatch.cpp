#include "gran/app/internal/app_state.hpp"

namespace gran
{
    /*
    ** С маршрутизатором — его доставка (уведомления о фокусе, сменённом обработчиком, уходят сразу);
    ** до run() — та же доставка через окно узла: так события можно проверять и без окна ОС.
    **
    ** With a router — its delivery (the notifications about the focus changed by a handler go at once);
    ** before run() — the same delivery through the node window: so events can be checked without an OS window as well.
    */
    bool Application::dispatchEvent(ui::Node& target, const std::string& type, const std::string& detail, bool bubbles)
    {
        ui::UiEvent event = ui::makeEvent(ui::eventType(type), target, type);
        input::InputRouter* router = app::routerOf(*_state, target);

        event.detail = detail;
        event.bubbles = bubbles;
        if (router != nullptr)
            return router->dispatch(event);
        return input::dispatchEvent(event, app::windowOf(*_state, target));
    }

    // Захват держит маршрутизатор окна узла / The capture is held by the router of the node window
    void Application::capturePointer(ui::Node& node)
    {
        input::InputRouter* router = app::routerOf(*_state, node);

        if (router != nullptr)
            router->capturePointer(&node);
    }

    // Чужой захват не трогается / A foreign capture is left alone
    void Application::releasePointer(ui::Node& node)
    {
        input::InputRouter* router = app::routerOf(*_state, node);

        if (router != nullptr && router->pointerCapture() == &node)
            router->capturePointer(nullptr);
    }
}
