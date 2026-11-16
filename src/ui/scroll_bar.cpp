#include "gran/ui/node.hpp"

#include <algorithm>

namespace gran::ui
{
    namespace
    {
        // Толщина бегунка, отступ от края и ширина полосы, где ловится мышь / The thumb thickness, the inset from the edge and the bar width where the mouse is caught
        const float THUMB_THICKNESS = 6.0f;
        const float THUMB_INSET = 2.0f;
        const float TRACK_THICKNESS = 10.0f;

        // Наименьшая длина бегунка: короче его трудно схватить / The smallest thumb length: a shorter one is hard to grab
        const float THUMB_MIN_LENGTH = 24.0f;

        // Прямоугольники полосы и бегунка по месту бегунка place вдоль полосы / The bar and thumb rectangles by the thumb place along the bar
        void placeBar(const Rect& view, bool vertical, float place, float length, ScrollBar& bar) noexcept
        {
            float right = view.x + view.width;
            float bottom = view.y + view.height;

            if (vertical)
            {
                bar.track = Rect{right - TRACK_THICKNESS, view.y, TRACK_THICKNESS, view.height};
                bar.thumb = Rect{right - THUMB_THICKNESS - THUMB_INSET, view.y + place, THUMB_THICKNESS, length};
                return;
            }
            bar.track = Rect{view.x, bottom - TRACK_THICKNESS, view.width, TRACK_THICKNESS};
            bar.thumb = Rect{view.x + place, bottom - THUMB_THICKNESS - THUMB_INSET, length, THUMB_THICKNESS};
        }
    }

    /*
    ** Полоса — у правого (вертикальная) или нижнего (горизонтальная) края видимой части без обводки.
    ** Длина бегунка — доля видимой части, путь бегунка пропорционален смещению прокрутки; смещение
    ** за пределом (до следующей компоновки) не выводит бегунок за полосу.
    **
    ** The bar is at the right (vertical) or bottom (horizontal) edge of the visible part without the border.
    ** The thumb length is the visible share, the thumb travel is proportional to the scroll offset; an offset
    ** beyond the limit (until the next layout) does not move the thumb out of the bar.
    */
    bool scrollBar(const Node& node, bool vertical, ScrollBar& bar) noexcept
    {
        Rect view = rectDeflate(node.layout.frame, node.computed.borderWidth);
        float viewLength = view.width;
        float contentLength = node.layout.contentSize.width;
        float offset = node.layout.scrollOffset.x;

        if (node.kind != ElementKind::SCROLL && node.kind != ElementKind::TEXT_AREA)
            return false;
        if (vertical)
        {
            viewLength = view.height;
            contentLength = node.layout.contentSize.height;
            offset = node.layout.scrollOffset.y;
        }
        if (!(contentLength > viewLength + 0.5f) || !(viewLength > 0.0f))
            return false;
        float length = std::max(THUMB_MIN_LENGTH, viewLength * viewLength / contentLength);
        bar.range = contentLength - viewLength;
        bar.travel = std::max(0.0f, viewLength - length);
        bar.view = viewLength;
        float place = bar.travel * std::min(1.0f, std::max(0.0f, offset / bar.range));
        placeBar(view, vertical, place, length, bar);
        return true;
    }

    // Обратное отображение: место бегунка → смещение прокрутки (в пределах [0, range]) / The inverse mapping: the thumb place → the scroll offset (within [0, range])
    float scrollForThumb(const ScrollBar& bar, float place) noexcept
    {
        if (!(bar.travel > 0.0f))
            return 0.0f;
        float share = std::min(1.0f, std::max(0.0f, place / bar.travel));
        return share * bar.range;
    }
}
