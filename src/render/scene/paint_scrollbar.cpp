#include "gran/render/internal/scene_internal.hpp"

#include <algorithm>

namespace gran::render::internal
{
    namespace
    {
        // Бегунок: прямоугольник со скруглением в половину ширины цветом текста с прозрачностью
        // A thumb: a rectangle rounded by half its width in the text color with transparency
        void emitThumb(const Rect& rect, const Color& color, const PaintState& state, SceneContext& context)
        {
            float unit = 1.0f;
            GpuInstance instance = placeInstance(state, rect, context.scale, unit);
            float radius = std::min(instance.rect[2], instance.rect[3]) * 0.5f;

            instance.kind = KIND_RECT;
            instance.fill = colorPackPremultiplied(colorWithOpacity(color, 0.35f));
            for (float& corner : instance.radii)
                corner = radius;
            context.scene.instances.push_back(instance);
        }
    }

    /*
    ** Полосы прокрутки поверх содержимого (как оверлейные полосы macOS и Windows 11):
    ** толщина 6, отступ 2, длина бегунка — доля видимой части, но не меньше 24.
    **
    ** Scrollbars over the content (as the overlay scrollbars of macOS and Windows 11):
    ** thickness 6, inset 2, the thumb length is the visible share but at least 24.
    */
    void paintScrollbars(const ui::Node& node, const PaintState& state, SceneContext& context)
    {
        ui::ScrollBar bar;

        // Геометрия общая с вводом мыши (ui::scrollBar): бегунок рисуется там, где его можно схватить
        // The geometry is shared with the mouse input (ui::scrollBar): the thumb is painted where it can be grabbed
        if (ui::scrollBar(node, true, bar))
            emitThumb(bar.thumb, node.computed.color, state, context);
        if (ui::scrollBar(node, false, bar))
            emitThumb(bar.thumb, node.computed.color, state, context);
    }
}
