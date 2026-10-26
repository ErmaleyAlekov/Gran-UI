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
            float space = mainAvailable - stack.computed.spacing * static_cast<float>(last - first - 1);

            for (std::size_t index = first; index < last; ++index)
                space -= items[index].marginMain;
            return space;
        }

        // Поперечный размер строки — наибольший элемент с отступами / The line cross size is the largest item with margins
        float lineCross(const std::vector<FlexItem>& items, std::size_t first, std::size_t last) noexcept
        {
            float cross = 0.0f;

            for (std::size_t index = first; index < last; ++index)
                cross = std::max(cross, items[index].cross + items[index].marginCross);
            return cross;
        }

        // Главный размер строки после разрешения / The line main size after resolution
        float lineMain(const ui::Node& stack, const std::vector<FlexItem>& items, std::size_t first, std::size_t last)
        {
            float main = stack.computed.spacing * static_cast<float>(last - first - 1);

            for (std::size_t index = first; index < last; ++index)
                main += items[index].target + items[index].marginMain;
            return main;
        }
    }

    /*
    ** Размер содержимого стека: при конечном месте гибкие длины разрешаются полностью — сжатие
    ** и рост (стек с растущим ребёнком занимает всё место, как блочный flex-контейнер в CSS), затем
    ** поперечные размеры перемеряются по итоговым длинам: перенос текста растущего поля даёт ту же
    ** высоту, что и при размещении.
    **
    ** The stack content size: with a finite space the flexible lengths are resolved fully — shrinking
    ** and growing (a stack with a growing child takes all the space, as a block flex container in CSS), then
    ** the cross sizes are measured again at the final lengths: the text wrapping of a growing field gives the same
    ** height as at placement.
    */
    Size measureStack(ui::Node& stack, Size contentAvailable, LayoutPass& pass)
    {
        bool horizontal = stack.computed.orientation == static_cast<int>(style::Orientation::HORIZONTAL);
        float mainAvailable = mainOf(contentAvailable, horizontal);
        std::vector<FlexItem> items;
        float totalMain = 0.0f;
        float totalCross = 0.0f;

        collectFlexItems(stack, contentAvailable, items, pass);
        if (items.empty())
            return Size{0.0f, 0.0f};
        std::vector<std::size_t> bounds = splitFlexLines(stack, items, mainAvailable);
        for (std::size_t line = 0; line + 1 < bounds.size(); ++line)
        {
            float space = lineSpace(stack, items, bounds[line], bounds[line + 1], mainAvailable);
            resolveFlexibleLengths(items, bounds[line], bounds[line + 1], space);
        }
        measureFlexCross(items, horizontal, crossOf(contentAvailable, horizontal), pass);
        for (std::size_t line = 0; line + 1 < bounds.size(); ++line)
        {
            totalMain = std::max(totalMain, lineMain(stack, items, bounds[line], bounds[line + 1]));
            if (line > 0)
                totalCross += stack.computed.spacing;
            totalCross += lineCross(items, bounds[line], bounds[line + 1]);
        }
        return sizeFrom(totalMain, totalCross, horizontal);
    }

    // Размещение: рост и сжатие по месту, затем строки одна за другой / Placement: growing and shrinking to fit, then the lines one by one
    void arrangeStack(ui::Node& stack, Rect content, LayoutPass& pass)
    {
        bool horizontal = stack.computed.orientation == static_cast<int>(style::Orientation::HORIZONTAL);
        Size available{content.width, content.height};
        std::vector<FlexItem> items;
        FlexLine line;

        collectFlexItems(stack, available, items, pass);
        if (items.empty())
            return;
        std::vector<std::size_t> bounds = splitFlexLines(stack, items, mainOf(available, horizontal));
        for (std::size_t index = 0; index + 1 < bounds.size(); ++index)
            resolveFlexibleLengths(items, bounds[index], bounds[index + 1],
                lineSpace(stack, items, bounds[index], bounds[index + 1], mainOf(available, horizontal)));
        measureFlexCross(items, horizontal, crossOf(available, horizontal), pass);
        for (std::size_t index = 0; index + 1 < bounds.size(); ++index)
        {
            line.first = bounds[index];
            line.last = bounds[index + 1];
            // Единственная строка занимает весь стек по поперечной оси (как в CSS), даже если дети шире
            // The single line takes the whole stack along the cross axis (as in CSS), even if the children are wider
            line.crossSize = lineCross(items, line.first, line.last);
            if (bounds.size() == 2)
                line.crossSize = crossOf(available, horizontal);
            placeFlexLine(stack, items, line, content, pass);
            line.crossOffset += line.crossSize + stack.computed.spacing;
        }
    }
}
