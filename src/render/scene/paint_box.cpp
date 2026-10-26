#include "gran/render/internal/scene_internal.hpp"

namespace gran::render::internal
{
    namespace
    {
        // Радиусы и рамка блока в экземпляр / The box radii and border into an instance
        void applyShape(GpuInstance& instance, const style::ComputedStyle& style, float unit)
        {
            CornerRadii radii = fitRadii(style.borderRadius, instance.rect[2], instance.rect[3], unit);

            instance.radii[0] = radii.topLeft;
            instance.radii[1] = radii.topRight;
            instance.radii[2] = radii.bottomRight;
            instance.radii[3] = radii.bottomLeft;
        }

        /*
        ** Тень box-shadow: фигура блока, сигма = размытие / 2 (как в браузерах), смещение
        ** и растяжение в extra. Прозрачная тень не рисуется.
        **
        ** A box-shadow: the box shape, sigma = blur / 2 (as in browsers), the offset
        ** and the spread in extra. A transparent shadow is not drawn.
        */
        void paintShadow(const ui::Node& node, const style::Shadow& shadow, const PaintState& state, SceneContext& context)
        {
            float unit = 1.0f;

            if (shadow.color.a <= 0.0f)
                return;
            GpuInstance instance = placeInstance(state, node.layout.frame, context.scale, unit);
            applyShape(instance, node.computed, unit);
            instance.kind = KIND_SHADOW;
            if (shadow.inset)
                instance.kind = KIND_INSET;
            instance.param = shadow.blur * 0.5f * unit;
            instance.extra[0] = shadow.offsetX * unit;
            instance.extra[1] = shadow.offsetY * unit;
            instance.extra[2] = shadow.spread * unit;
            instance.fill = colorPackPremultiplied(shadow.color);
            context.scene.instances.push_back(instance);
        }

        // Фон и рамка одним экземпляром / The background and the border in one instance
        void paintBackground(const ui::Node& node, const PaintState& state, SceneContext& context)
        {
            const style::ComputedStyle& style = node.computed;
            const Insets& border = style.borderWidth;
            bool bordered = style.borderColor.a > 0.0f
                && (border.top > 0.0f || border.right > 0.0f || border.bottom > 0.0f || border.left > 0.0f);
            float unit = 1.0f;

            if (style.background.kind == style::PaintKind::NONE && !bordered)
                return;
            GpuInstance instance = placeInstance(state, node.layout.frame, context.scale, unit);
            applyShape(instance, style, unit);
            instance.kind = KIND_RECT;
            applyPaint(instance, style.background, context);
            if (bordered)
            {
                instance.border[0] = border.top * unit;
                instance.border[1] = border.right * unit;
                instance.border[2] = border.bottom * unit;
                instance.border[3] = border.left * unit;
                instance.stroke = colorPackPremultiplied(style.borderColor);
            }
            context.scene.instances.push_back(instance);
        }
    }

    // Подложка — до теней (своя тень блока в размытие не входит). CSS рисует первую тень сверху:
    // внешние — в обратном порядке, затем фон, затем внутренние.
    // The backdrop goes before the shadows (the box own shadow is not part of the blur). CSS paints the first shadow
    // on top: the outer ones in reverse order, then the background, then the inner ones.
    void paintBox(const ui::Node& node, const PaintState& state, SceneContext& context)
    {
        const std::vector<style::Shadow>& shadows = node.computed.shadows;

        if (node.computed.backdropBlur > 0.0f)
            paintBackdrop(node, state, context);
        for (std::size_t index = shadows.size(); index > 0; --index)
        {
            if (!shadows[index - 1].inset)
                paintShadow(node, shadows[index - 1], state, context);
        }
        paintBackground(node, state, context);
        for (std::size_t index = shadows.size(); index > 0; --index)
        {
            if (shadows[index - 1].inset)
                paintShadow(node, shadows[index - 1], state, context);
        }
    }
}
