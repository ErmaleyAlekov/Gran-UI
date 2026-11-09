#include "gran/render/internal/scene_internal.hpp"

#include <algorithm>
#include <cmath>

namespace gran::render::internal
{
    /*
    ** Ручка размера окна — шесть круглых точек лесенкой к правому нижнему углу (три в нижнем ряду, две, одна),
    ** как у строки состояния Windows; цвет — цвет текста узла (стиль темы SizeGrip).
    **
    ** The window size grip is six round dots in steps toward the bottom-right corner (three in the bottom row, two, one),
    ** as in the Windows status bar; the color is the node text color (the SizeGrip theme style).
    */
    void paintGrip(const ui::Node& node, const PaintState& state, SceneContext& context)
    {
        Rect content = contentRect(node);
        float step = std::floor(std::min(content.width, content.height) / 3.0f);
        float dot = std::max(1.5f, std::round(step * 0.5f));
        ControlBox box{Rect{0.0f, 0.0f, dot, dot}, dot * 0.5f, node.computed.color, Color{}, 0.0f};

        if (step <= 0.0f)
            return;
        for (int row = 0; row < 3; ++row)
        {
            for (int column = 2 - row; column < 3; ++column)
            {
                box.rect.x = content.x + content.width - step * static_cast<float>(3 - column) + (step - dot) * 0.5f;
                box.rect.y = content.y + content.height - step * static_cast<float>(3 - row) + (step - dot) * 0.5f;
                emitBox(box, 0.0f, state, context);
            }
        }
    }
}
