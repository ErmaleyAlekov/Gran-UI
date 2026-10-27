#include "gran/layout/internal/layout_internal.hpp"

#include <algorithm>

namespace gran::layout::internal
{
    // Место под стрелку подменю — в кеглях (стрелку рисует сцена у правого края) / The room for the submenu arrow — in font sizes (the scene draws the arrow at the right edge)
    constexpr float SUBMENU_MARK = 1.6f;

    /*
    ** Подпись меряется по ширине без места под стрелку: длинная подпись переносится, а не
    ** заходит под неё; ширина меню (самый широкий пункт) так учитывает и стрелку.
    **
    ** The label is measured by the width without the arrow room: a long label wraps instead of
    ** going under it; the menu width (the widest item) thus accounts for the arrow too.
    */
    Size menuItemSize(ui::Node& node, Size contentAvailable, LayoutPass& pass)
    {
        float mark = 0.0f;

        if (ui::submenuOf(node) != nullptr)
            mark = node.computed.fontSize * SUBMENU_MARK;
        Size label = pass.measurer.measureText(node, std::max(0.0f, contentAvailable.width - mark));
        return Size{label.width + mark, label.height};
    }
}
