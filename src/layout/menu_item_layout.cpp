#include "gran/layout/internal/layout_internal.hpp"

#include <algorithm>
#include <cmath>
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

        // У пункта строки меню стрелки нет: его меню открывается вниз / A menu bar item has no arrow: its menu opens downward
        if (ui::submenuOf(node) != nullptr && !ui::isMenuBarItem(node))
            mark = node.computed.fontSize * SUBMENU_MARK;
        if (!node.content.shortcut.empty())
            mark += pass.measurer.measureText(ui::shortcutLabel(node), std::numeric_limits<float>::infinity()).width
                + node.computed.fontSize * ui::SHORTCUT_GAP;
        Size label = pass.measurer.measureText(node, std::max(0.0f, contentAvailable.width - mark));
        return Size{label.width + mark, label.height};
    }

    /*
    ** Разделитель — линия в один пиксель: в горизонтальном родителе (панель) — вертикальная, в вертикальном
    ** (меню) — горизонтальная; длину по другой оси даёт растяжение (alignSelf="stretch" в теме). Кнопка «ещё» —
    ** квадрат под стрелку высотой в строку текста.
    **
    ** A separator is a one pixel line: in a horizontal parent (a toolbar) — vertical, in a vertical one
    ** (a menu) — horizontal; the length along the other axis comes from stretching (alignSelf="stretch" in the theme). The "more" button is
    ** a square for the arrow as high as a text line.
    */
    Size toolLeafSize(const ui::Node& node) noexcept
    {
        const ui::Node* parent = node.parent;
        float line = std::round(node.computed.fontSize * node.computed.lineHeight);

        if (node.kind == ui::ElementKind::TOOL_MORE)
            return Size{std::round(node.computed.fontSize * 1.2f), line};
        if (parent != nullptr && parent->computed.orientation == static_cast<int>(style::Orientation::HORIZONTAL))
            return Size{1.0f, 0.0f};
        return Size{0.0f, 1.0f};
    }
}
