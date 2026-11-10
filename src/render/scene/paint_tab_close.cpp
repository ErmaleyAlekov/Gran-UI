#include "gran/render/internal/scene_internal.hpp"

#include <algorithm>
#include <cmath>

namespace gran::render::internal
{
    /*
    ** Крестик закрытия вкладки документа — две полосы у правого края содержимого заголовка, в квадрате
    ** ui::TAB_CLOSE_SIZE кеглей (то же место проверяет ввод); приглушён, как стрелка подменю.
    **
    ** The document tab close cross is two bars at the right edge of the header content, in a square of
    ** ui::TAB_CLOSE_SIZE font sizes (the input checks the same place); muted, as the submenu arrow.
    */
    void paintTabClose(const ui::Node& header, const PaintState& state, SceneContext& context)
    {
        Rect content = contentRect(header);
        float box = header.computed.fontSize * ui::TAB_CLOSE_SIZE;
        float arm = std::round(box * 0.25f);
        Vec2 center{content.x + content.width - box * 0.5f, content.y + content.height * 0.5f};
        Color color = colorWithOpacity(header.computed.color, 0.65f);
        float thickness = std::max(1.0f, std::round(header.computed.fontSize * 0.1f));

        emitBar(Bar{Vec2{center.x - arm, center.y - arm}, Vec2{center.x + arm, center.y + arm}, thickness}, color, state, context);
        emitBar(Bar{Vec2{center.x - arm, center.y + arm}, Vec2{center.x + arm, center.y - arm}, thickness}, color, state, context);
    }
}
