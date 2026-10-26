#include "gran/render/internal/scene_internal.hpp"

namespace gran::render::internal
{
    /*
    ** Подложка (backdrop-filter: blur): текущий сегмент закрывается, новый начинается с
    ** размытия всего, что нарисовано до него. Экземпляр KIND_BACKDROP без тонировки
    ** (fill = 0) кладёт размытую копию в форму блока; полупрозрачный фон блока рисуется
    ** поверх обычным экземпляром. Радиус переводится в пиксели устройства.
    **
    ** A backdrop (backdrop-filter: blur): the current segment is closed, a new one starts with
    ** blurring everything drawn before it. A KIND_BACKDROP instance without a tint
    ** (fill = 0) puts the blurred copy into the box shape; the translucent box background is drawn
    ** on top by a regular instance. The radius is converted into device pixels.
    */
    void paintBackdrop(const ui::Node& node, const PaintState& state, SceneContext& context)
    {
        float unit = 1.0f;
        GpuInstance instance = placeInstance(state, node.layout.frame, context.scale, unit);
        CornerRadii radii = fitRadii(node.computed.borderRadius, instance.rect[2], instance.rect[3], unit);

        closeSegment(context.scene);
        context.scene.segments.push_back(rhi::FrameSegment{static_cast<std::uint32_t>(context.scene.instances.size()),
            0, true, node.computed.backdropBlur * context.scale});
        instance.kind = KIND_BACKDROP;
        instance.radii[0] = radii.topLeft;
        instance.radii[1] = radii.topRight;
        instance.radii[2] = radii.bottomRight;
        instance.radii[3] = radii.bottomLeft;
        instance.fill = 0;
        context.scene.instances.push_back(instance);
    }

    // Число экземпляров последнего сегмента — до конца массива / The last segment instance count — up to the array end
    void closeSegment(Scene& scene)
    {
        rhi::FrameSegment& last = scene.segments.back();

        last.count = static_cast<std::uint32_t>(scene.instances.size()) - last.first;
    }
}
