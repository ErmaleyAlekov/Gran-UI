#include "gran/render/internal/scene_internal.hpp"

#include <algorithm>

namespace gran::render::internal
{
    namespace
    {
        // Место картинки и видимая часть текстуры / The image place and the visible texture part
        struct Placement
        {
            Rect target;
            float uv[4];
        };

        /*
        ** fit как object-fit в CSS: fill — растянуть; contain — вписать целиком по центру;
        ** cover — заполнить блок с обрезкой краёв; none — собственный размер по центру (с обрезкой).
        **
        ** fit as object-fit in CSS: fill — stretch; contain — fit entirely in the center;
        ** cover — fill the box cropping the edges; none — the intrinsic size in the center (with cropping).
        */
        Placement placeImage(const Rect& box, float width, float height, ui::ImageFit fit)
        {
            Placement placement{box, {0.0f, 0.0f, 1.0f, 1.0f}};
            float scale = 1.0f;

            if (fit == ui::ImageFit::FILL)
                return placement;
            if (fit == ui::ImageFit::CONTAIN)
                scale = std::min(box.width / width, box.height / height);
            else if (fit == ui::ImageFit::COVER)
                scale = std::max(box.width / width, box.height / height);
            float shownWidth = std::min(box.width, width * scale);
            float shownHeight = std::min(box.height, height * scale);
            float cropX = (1.0f - shownWidth / (width * scale)) * 0.5f;
            float cropY = (1.0f - shownHeight / (height * scale)) * 0.5f;
            placement.target = Rect{box.x + (box.width - shownWidth) * 0.5f, box.y + (box.height - shownHeight) * 0.5f,
                shownWidth, shownHeight};
            placement.uv[0] = cropX;
            placement.uv[1] = cropY;
            placement.uv[2] = 1.0f - cropX;
            placement.uv[3] = 1.0f - cropY;
            return placement;
        }
    }

    // Картинка с радиусами скругления блока / An image with the box rounding radii
    void paintImage(const ui::Node& node, const PaintState& state, SceneContext& context)
    {
        const ImageEntry& entry = context.resources.images.get(node.content.source);
        float unit = 1.0f;

        if (entry.failed || entry.width == 0 || entry.height == 0)
            return;
        Rect box = contentRect(node);
        Placement placement = placeImage(box, static_cast<float>(entry.width), static_cast<float>(entry.height),
            node.content.fit);
        GpuInstance instance = placeInstance(state, placement.target, context.scale, unit);
        CornerRadii radii = fitRadii(node.computed.borderRadius, instance.rect[2], instance.rect[3], unit);
        instance.kind = KIND_IMAGE;
        instance.paint[0] = entry.slot;
        for (int index = 0; index < 4; ++index)
            instance.extra[index] = placement.uv[index];
        instance.radii[0] = radii.topLeft;
        instance.radii[1] = radii.topRight;
        instance.radii[2] = radii.bottomRight;
        instance.radii[3] = radii.bottomLeft;
        context.scene.instances.push_back(instance);
    }
}
