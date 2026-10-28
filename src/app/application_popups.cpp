#include "gran/app/internal/app_state.hpp"

namespace gran
{
    /*
    ** Якорь — описанный прямоугольник узла в координатах окна (с учётом поворотов и масштабов
    ** предков). До run() маршрутизатора нет: слой открывать некому, вызов ничего не делает.
    **
    ** The anchor is the node bounding rectangle in window coordinates (with the ancestor rotations
    ** and scales). Before run() there is no router: nobody can open the layer, the call does nothing.
    */
    void Application::showPopup(ui::Node& popup, const ui::Node& anchor)
    {
        input::InputRouter* router = app::routerOf(*_state, popup);

        if (router == nullptr)
            return;
        router->openPopup(popup, affineBounds(ui::nodeToWindow(anchor), anchor.layout.frame), false);
    }

    // Якорь-точка: слой встаёт правым нижним углом курсора, у краёв окна — отражается / A point anchor: the layer stands at the cursor bottom right, at the window edges it is mirrored
    void Application::showPopupAt(ui::Node& popup, Vec2 point)
    {
        input::InputRouter* router = app::routerOf(*_state, popup);

        if (router == nullptr)
            return;
        router->openPopup(popup, Rect{point.x, point.y, 0.0f, 0.0f}, false);
    }

    // Фокус из слоя возвращается туда, где был до открытия / The focus from the layer returns where it was before opening
    void Application::hidePopup(ui::Node& popup)
    {
        input::InputRouter* router = app::routerOf(*_state, popup);

        if (router != nullptr)
            router->closePopup(popup);
    }
}
