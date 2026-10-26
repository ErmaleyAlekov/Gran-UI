#include "gran/layout/internal/layout_internal.hpp"

#include <algorithm>

namespace gran::layout::internal
{
    namespace
    {
        // Начальное смещение и добавка к промежутку по justify / The start offset and the gap addition by justify
        void justifyOffsets(int justify, float leftover, std::size_t count, float& start, float& gap) noexcept
        {
            float n = static_cast<float>(count);

            start = 0.0f;
            gap = 0.0f;
            if (leftover <= 0.0f || count == 0)
                return;
            if (justify == static_cast<int>(style::Justify::END))
                start = leftover;
            else if (justify == static_cast<int>(style::Justify::CENTER))
                start = leftover / 2.0f;
            else if (justify == static_cast<int>(style::Justify::SPACE_BETWEEN) && count > 1)
                gap = leftover / (n - 1.0f);
            else if (justify == static_cast<int>(style::Justify::SPACE_AROUND))
            {
                gap = leftover / n;
                start = gap / 2.0f;
            }
            else if (justify == static_cast<int>(style::Justify::SPACE_EVENLY))
            {
                gap = leftover / (n + 1.0f);
                start = gap;
            }
        }

        /*
        ** Поперечный размер и смещение элемента в строке. stretch растягивает только
        ** элемент без явного поперечного размера, с учётом его min/max.
        **
        ** The cross size and offset of an item in a line. stretch stretches only
        ** an item without an explicit cross size, respecting its min/max.
        */
        void crossPlacement(const ui::Node& stack, const FlexItem& item, float lineCross, float& size, float& offset)
        {
            bool horizontal = stack.computed.orientation == static_cast<int>(style::Orientation::HORIZONTAL);
            const style::ComputedStyle& child = item.node->computed;
            int align = child.alignSelf;
            float room = std::max(0.0f, lineCross - item.marginCross);

            if (align == static_cast<int>(style::Align::AUTO))
                align = stack.computed.align;
            size = item.cross;
            offset = 0.0f;
            if (align == static_cast<int>(style::Align::STRETCH) && !hasFixedSize(*item.node, !horizontal))
            {
                if (horizontal)
                    size = clampSize(room, child.minHeight, child.maxHeight);
                else
                    size = clampSize(room, child.minWidth, child.maxWidth);
            }
            else if (align == static_cast<int>(style::Align::CENTER))
                offset = (room - size) / 2.0f;
            else if (align == static_cast<int>(style::Align::END))
                offset = room - size;
        }

        // Начальные внешние отступы по осям (лево/верх) / The start margins along the axes (left/top)
        Vec2 startMargins(const ui::Node& node, bool horizontal) noexcept
        {
            if (horizontal)
                return Vec2{node.computed.margin.left, node.computed.margin.top};
            return Vec2{node.computed.margin.top, node.computed.margin.left};
        }
    }

    // Элементы строки по порядку: позиция по главной оси растёт на размер, отступы и промежуток
    // The line items in order: the main position grows by the size, the margins and the gap
    void placeFlexLine(ui::Node& stack, const std::vector<FlexItem>& items, const FlexLine& line, Rect content,
        LayoutPass& pass)
    {
        bool horizontal = stack.computed.orientation == static_cast<int>(style::Orientation::HORIZONTAL);
        float used = stack.computed.spacing * static_cast<float>(line.last - line.first - 1);
        float start = 0.0f;
        float gap = 0.0f;

        for (std::size_t index = line.first; index < line.last; ++index)
            used += items[index].target + items[index].marginMain;
        justifyOffsets(stack.computed.justify, mainOf(Size{content.width, content.height}, horizontal) - used,
            line.last - line.first, start, gap);
        float main = start;
        for (std::size_t index = line.first; index < line.last; ++index)
        {
            const FlexItem& item = items[index];
            float crossSize = 0.0f;
            float crossOffset = 0.0f;
            crossPlacement(stack, item, line.crossSize, crossSize, crossOffset);
            Vec2 margin = startMargins(*item.node, horizontal);
            Size size = sizeFrom(item.target, crossSize, horizontal);
            Size position = sizeFrom(main + margin.x, line.crossOffset + crossOffset + margin.y, horizontal);
            arrangeNode(*item.node, Rect{content.x + position.width, content.y + position.height,
                size.width, size.height}, pass);
            main += item.target + item.marginMain + stack.computed.spacing + gap;
        }
    }
}
