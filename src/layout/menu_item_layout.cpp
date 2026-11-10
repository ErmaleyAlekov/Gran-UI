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
    ** квадрат под стрелку высотой в строку текста, ручка размера окна — квадрат в кегль, ручка разделителя — полоса
    ** в 0,4 кегля (не тоньше 3 пикселей) поперёк оси разделителя.
    **
    ** A separator is a one pixel line: in a horizontal parent (a toolbar) — vertical, in a vertical one
    ** (a menu) — horizontal; the length along the other axis comes from stretching (alignSelf="stretch" in the theme). The "more" button is
    ** a square for the arrow as high as a text line, the window size grip is a square of the font size, the splitter handle is a strip
    ** of 0.4 font sizes (not thinner than 3 pixels) across the splitter axis.
    */
    Size toolLeafSize(const ui::Node& node) noexcept
    {
        const ui::Node* parent = node.parent;
        float line = std::round(node.computed.fontSize * node.computed.lineHeight);

        if (node.kind == ui::ElementKind::TOOL_MORE)
            return Size{std::round(node.computed.fontSize * 1.2f), line};
        if (node.kind == ui::ElementKind::SIZE_GRIP)
            return Size{std::round(node.computed.fontSize), std::round(node.computed.fontSize)};
        float thickness = 1.0f;
        if (node.kind == ui::ElementKind::SPLITTER_HANDLE)
            thickness = std::max(3.0f, std::round(node.computed.fontSize * 0.4f));
        if (parent != nullptr && parent->computed.orientation == static_cast<int>(style::Orientation::HORIZONTAL))
            return Size{thickness, 0.0f};
        return Size{0.0f, thickness};
    }

    // Крестик не перекрывает подпись: место под него вычитается из ширины переноса и прибавляется к размеру
    // The cross does not cover the label: its room is taken from the wrap width and added to the size
    Size tabHeaderSize(ui::Node& node, Size contentAvailable, LayoutPass& pass)
    {
        float mark = 0.0f;

        if (ui::tabClosable(node))
            mark = node.computed.fontSize * ui::TAB_CLOSE_SIZE;
        Size label = pass.measurer.measureText(node, std::max(0.0f, contentAvailable.width - mark));
        return Size{label.width + mark, label.height};
    }
}
