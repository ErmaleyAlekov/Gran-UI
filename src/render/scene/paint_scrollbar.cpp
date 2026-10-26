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
        Rect view = rectDeflate(node.layout.frame, node.computed.borderWidth);
        Size content = node.layout.contentSize;
        Vec2 offset = node.layout.scrollOffset;

        if (node.kind != ui::ElementKind::SCROLL && node.kind != ui::ElementKind::TEXT_AREA)
            return;
        if (content.height > view.height + 0.5f)
        {
            float length = std::max(24.0f, view.height * view.height / content.height);
            float travel = (view.height - length) * offset.y / (content.height - view.height);
            emitThumb(Rect{view.x + view.width - 8.0f, view.y + travel, 6.0f, length}, node.computed.color, state, context);
        }
        if (content.width > view.width + 0.5f)
        {
            float length = std::max(24.0f, view.width * view.width / content.width);
            float travel = (view.width - length) * offset.x / (content.width - view.width);
            emitThumb(Rect{view.x + travel, view.y + view.height - 8.0f, length, 6.0f}, node.computed.color, state, context);
        }
    }
}
