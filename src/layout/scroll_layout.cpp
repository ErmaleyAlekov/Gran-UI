#include "gran/layout/internal/layout_internal.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace gran::layout::internal
{
    namespace
    {
        // Доступный размер для содержимого: по оси прокрутки — без предела
        // The available size for the content: unbounded along the scroll axis
        Size contentRoom(const ui::Node& scroll, Size available) noexcept
        {
            const float unbounded = std::numeric_limits<float>::infinity();
            ui::ScrollDirection direction = scroll.content.direction;

            if (direction != ui::ScrollDirection::HORIZONTAL)
                available.height = unbounded;
            if (direction != ui::ScrollDirection::VERTICAL)
                available.width = unbounded;
            return available;
        }

        // Единственный видимый ребёнок прокрутки или nullptr / The single visible scroll child or nullptr
        ui::Node* scrollChild(ui::Node& scroll) noexcept
        {
            if (scroll.children.empty() || !ui::inFlow(*scroll.children.front()))
                return nullptr;
            return scroll.children.front().get();
        }
    }

    // Прокрутка не больше доступного места: лишнее уходит под прокрутку
    // A scroll is not larger than the available space: the excess goes under scrolling
    Size measureScroll(ui::Node& scroll, Size contentAvailable, LayoutPass& pass)
    {
        ui::Node* child = scrollChild(scroll);
        Size size{0.0f, 0.0f};

        if (child == nullptr)
            return size;
        size = measureNode(*child, shrinkAvailable(contentRoom(scroll, contentAvailable), child->computed.margin), pass);
        size.width = std::fmin(size.width + child->computed.margin.left + child->computed.margin.right,
            contentAvailable.width);
        size.height = std::fmin(size.height + child->computed.margin.top + child->computed.margin.bottom,
            contentAvailable.height);
        return size;
    }

    /*
    ** Содержимое не меньше окна прокрутки и сдвинуто на scrollOffset; смещение
    ** ограничивается так, чтобы за краем содержимого не оставалось пустоты.
    **
    ** The content is not smaller than the scroll viewport and shifted by scrollOffset; the offset
    ** is clamped so that no emptiness remains beyond the content edge.
    */
    void arrangeScroll(ui::Node& scroll, Rect content, LayoutPass& pass)
    {
        ui::Node* child = scrollChild(scroll);
        Vec2& offset = scroll.layout.scrollOffset;

        if (child == nullptr)
            return;
        const Insets& margin = child->computed.margin;
        Size size = measureNode(*child, shrinkAvailable(contentRoom(scroll, Size{content.width, content.height}),
            margin), pass);
        size.width = clampSize(std::fmax(size.width, content.width - margin.left - margin.right),
            child->computed.minWidth, child->computed.maxWidth);
        size.height = clampSize(std::fmax(size.height, content.height - margin.top - margin.bottom),
            child->computed.minHeight, child->computed.maxHeight);
        scroll.layout.contentSize = Size{size.width + margin.left + margin.right, size.height + margin.top + margin.bottom};
        offset.x = clampSize(offset.x, 0.0f, std::fmax(0.0f, scroll.layout.contentSize.width - content.width));
        offset.y = clampSize(offset.y, 0.0f, std::fmax(0.0f, scroll.layout.contentSize.height - content.height));
        arrangeNode(*child, Rect{content.x + margin.left - offset.x, content.y + margin.top - offset.y,
            size.width, size.height}, pass);
    }
}
