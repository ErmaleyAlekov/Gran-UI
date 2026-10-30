#include "gran/layout/internal/layout_internal.hpp"

#include <algorithm>
#include <limits>

namespace gran::layout::internal
{
    // Место под стрелку подменю — в кеглях (стрелку рисует сцена у правого края) / The room for the submenu arrow — in font sizes (the scene draws the arrow at the right edge)
    constexpr float SUBMENU_MARK = 1.6f;

    /*
    ** Подпись меряется по ширине без места под стрелку и подпись сочетания (Ctrl+S справа): длинная
    ** подпись переносится, а не заходит под них; ширина меню (самый широкий пункт) так учитывает и их.
    **
    ** The label is measured by the width without the room for the arrow and the shortcut label (Ctrl+S on the right): a long
    ** label wraps instead of going under them; the menu width (the widest item) thus accounts for them too.
    */
    Size menuItemSize(ui::Node& node, Size contentAvailable, LayoutPass& pass)
    {
        float mark = 0.0f;

        if (ui::submenuOf(node) != nullptr)
            mark = node.computed.fontSize * SUBMENU_MARK;
        if (!node.content.shortcut.empty())
            mark += pass.measurer.measureText(ui::shortcutLabel(node), std::numeric_limits<float>::infinity()).width
                + node.computed.fontSize * ui::SHORTCUT_GAP;
        Size label = pass.measurer.measureText(node, std::max(0.0f, contentAvailable.width - mark));
        return Size{label.width + mark, label.height};
    }
}
