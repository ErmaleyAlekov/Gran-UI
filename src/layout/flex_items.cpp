#include "gran/layout/internal/layout_internal.hpp"

#include <algorithm>
#include <cmath>

namespace gran::layout::internal
{
    namespace
    {
        // Процентный размер по главной оси считается от стека, остальное меряется по содержимому
        // A percent size along the main axis is taken from the stack, the rest is measured by content
        float childMainAvailable(const ui::Node& child, bool horizontal, float mainAvailable) noexcept
        {
            style::LengthUnit unit = child.computed.height.unit;

            if (horizontal)
                unit = child.computed.width.unit;
            if (unit == style::LengthUnit::PERCENT)
                return mainAvailable;
            return std::numeric_limits<float>::infinity();
        }

        // Пределы ребёнка по главной оси / The child limits along the main axis
        void mainLimits(const ui::Node& child, bool horizontal, FlexItem& item) noexcept
        {
            item.minMain = child.computed.minHeight;
            item.maxMain = child.computed.maxHeight;
            if (horizontal)
            {
                item.minMain = child.computed.minWidth;
                item.maxMain = child.computed.maxWidth;
            }
        }
    }

    /*
    ** Гипотетический размер каждого видимого ребёнка: по главной оси — по содержимому
    ** (max-content), по поперечной — в пределах стека. Невидимые (visible=false) не участвуют.
    **
    ** The hypothetical size of every visible child: along the main axis by content
    ** (max-content), along the cross axis within the stack. Invisible ones (visible=false) do not take part.
    */
    void collectFlexItems(ui::Node& stack, Size contentAvailable, std::vector<FlexItem>& items, LayoutPass& pass)
    {
        bool horizontal = stack.computed.orientation == static_cast<int>(style::Orientation::HORIZONTAL);
        float crossAvailable = crossOf(contentAvailable, horizontal);
        float mainAvailable = mainOf(contentAvailable, horizontal);

        for (const std::unique_ptr<ui::Node>& child : stack.children)
        {
            if (!ui::inFlow(*child))
                continue;
            FlexItem item;
            item.node = child.get();
            item.marginMain = insetsMain(child->computed.margin, horizontal);
            item.marginCross = insetsCross(child->computed.margin, horizontal);
            float main = childMainAvailable(*child, horizontal, mainAvailable - item.marginMain);
            float cross = std::fmax(0.0f, crossAvailable - item.marginCross);
            Size measured = measureNode(*child, sizeFrom(main, cross, horizontal), pass);
            item.base = mainOf(measured, horizontal);
            item.cross = crossOf(measured, horizontal);
            mainLimits(*child, horizontal, item);
            // Явный размер по главной оси — ещё и минимум: элемент не сжимается (иконка 36 px остаётся 36 px)
            // An explicit main size is also the minimum: the item does not shrink (a 36 px icon stays 36 px)
            if (hasFixedSize(*child, horizontal))
                item.minMain = std::fmax(item.minMain, item.base);
            item.target = item.base;
            items.push_back(item);
        }
    }

    // Жадное разбиение на строки по базовым размерам / A greedy split into lines by the base sizes
    std::vector<std::size_t> splitFlexLines(const ui::Node& stack, const std::vector<FlexItem>& items, float available)
    {
        std::vector<std::size_t> bounds{0};
        float spacing = stack.computed.spacing;
        float lineMain = 0.0f;

        for (std::size_t index = 0; index < items.size(); ++index)
        {
            float size = items[index].base + items[index].marginMain;
            bool overflow = index > bounds.back() && lineMain + spacing + size > available;
            if (stack.computed.wrap && std::isfinite(available) && overflow)
            {
                bounds.push_back(index);
                lineMain = 0.0f;
            }
            if (index > bounds.back())
                lineMain += spacing;
            lineMain += size;
        }
        bounds.push_back(items.size());
        return bounds;
    }

    // Изменившийся главный размер меняет поперечный (перенос текста) — перемеряем
    // A changed main size changes the cross one (text wrapping) — measure again
    void measureFlexCross(std::vector<FlexItem>& items, bool horizontal, float crossAvailable, LayoutPass& pass)
    {
        for (FlexItem& item : items)
        {
            if (item.target == item.base)
                continue;
            float cross = std::fmax(0.0f, crossAvailable - item.marginCross);
            Size measured = measureNode(*item.node, sizeFrom(item.target, cross, horizontal), pass);
            item.cross = crossOf(measured, horizontal);
        }
    }
}
