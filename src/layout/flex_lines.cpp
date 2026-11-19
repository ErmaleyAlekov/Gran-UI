#include "gran/layout/internal/layout_internal.hpp"

#include <algorithm>
#include <cmath>

namespace gran::layout::internal
{
    namespace
    {
        // Место для рамок элементов строки: без внешних отступов и промежутков
        // The space for the line item frames: without the margins and the gaps
        float lineSpace(const ui::Node& stack, const std::vector<FlexItem>& items, std::size_t first,
            std::size_t last, float mainAvailable) noexcept
        {
            double space = static_cast<double>(mainAvailable) - stack.computed.spacing * static_cast<double>(last - first - 1);

            for (std::size_t index = first; index < last; ++index)
                space -= items[index].marginMain;
            return static_cast<float>(space);
        }

        // Прокручиваемый блок (scroll container в CSS): содержимое прокручивается или обрезается — автоминимум 0
        // A scrolled box (a CSS scroll container): the content scrolls or is clipped — the automatic minimum is 0
        bool scrollsContent(const ui::Node& node) noexcept
        {
            return node.kind == ui::ElementKind::SCROLL || node.kind == ui::ElementKind::DATA_GRID
                || node.kind == ui::ElementKind::TREE_VIEW
                || node.computed.overflow != static_cast<int>(style::Overflow::VISIBLE);
        }

        /*
        ** Автоматический минимум по вертикали (как min-height: auto у гибких элементов CSS): строка сжимается —
        ** элемент не становится ниже своего содержимого. Содержимое меряется при нулевой высоте: прокрутки внутри
        ** отдают всё место (их минимум 0), текст и поля — свою высоту. Только при сжатии: в обычном проходе
        ** лишних измерений нет.
        **
        ** The automatic vertical minimum (as min-height: auto of CSS flex items): when the line shrinks
        ** an item does not become lower than its content. The content is measured at a zero height: the scrolls inside
        ** give away all the room (their minimum is 0), texts and fields keep their height. Only when shrinking: an ordinary
        ** pass has no extra measurements.
        */
        void applyAutoMinimum(std::vector<FlexItem>& items, std::size_t first, std::size_t last, float space,
            LayoutPass& pass)
        {
            double sumBase = 0.0;

            for (std::size_t index = first; index < last; ++index)
                sumBase += items[index].base;
            if (!std::isfinite(space) || sumBase <= space)
                return;
            for (std::size_t index = first; index < last; ++index)
            {
                FlexItem& item = items[index];
                if (item.node->computed.shrink <= 0.0f || item.minMain >= item.base || scrollsContent(*item.node))
                    continue;
                float content = measureNode(*item.node, Size{item.crossRoom, 0.0f}, pass).height;
                item.minMain = std::max(item.minMain, std::min(content, item.base));
            }
        }
    }

    // Строки по очереди: место строки, автоминимум (вертикальный стек), разрешение гибких длин / The lines in turn: the line space, the automatic minimum (a vertical stack), resolving the flexible lengths
    void resolveFlexLines(const ui::Node& stack, std::vector<FlexItem>& items, const std::vector<std::size_t>& bounds,
        float mainAvailable, LayoutPass& pass)
    {
        bool horizontal = stack.computed.orientation == static_cast<int>(style::Orientation::HORIZONTAL);

        for (std::size_t line = 0; line + 1 < bounds.size(); ++line)
        {
            float space = lineSpace(stack, items, bounds[line], bounds[line + 1], mainAvailable);
            if (!horizontal)
                applyAutoMinimum(items, bounds[line], bounds[line + 1], space, pass);
            resolveFlexibleLengths(items, bounds[line], bounds[line + 1], space);
        }
    }
}
