#include "gran/render/internal/scene_internal.hpp"

#include <cmath>

namespace gran::render::internal
{
    /*
    ** Пока панель тянут за заголовок, хост рисует поверх детей полупрозрачную полосу цвета accentColor у края, куда
    ** она упадёт (четверть хоста), а для отрыва в окно (dropSide = DOCK_SIDES) — рамку вокруг центра.
    **
    ** While a panel is dragged by its title, the host draws over its children a semi-transparent accentColor band at the edge it
    ** will drop onto (a quarter of the host), and for tearing off into a window (dropSide = DOCK_SIDES) — a frame around the center.
    */
    void paintDockDrop(const ui::Node& host, const PaintState& state, SceneContext& context)
    {
        Rect area = contentRect(host);
        int side = host.content.dock.dropSide;
        Color accent = host.computed.accentColor;
        ControlBox box{area, 6.0f, colorWithOpacity(accent, 0.18f), colorWithOpacity(accent, 0.8f), 2.0f};

        if (side == static_cast<int>(ui::DockSide::LEFT) || side == static_cast<int>(ui::DockSide::RIGHT))
            box.rect.width = std::round(area.width * 0.25f);
        if (side == static_cast<int>(ui::DockSide::RIGHT))
            box.rect.x = area.x + area.width - box.rect.width;
        if (side == static_cast<int>(ui::DockSide::TOP) || side == static_cast<int>(ui::DockSide::BOTTOM))
            box.rect.height = std::round(area.height * 0.25f);
        if (side == static_cast<int>(ui::DockSide::BOTTOM))
            box.rect.y = area.y + area.height - box.rect.height;
        emitBox(box, 0.0f, state, context);
    }
}
