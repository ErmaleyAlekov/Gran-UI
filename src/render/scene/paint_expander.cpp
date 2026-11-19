#include "gran/render/internal/scene_internal.hpp"
#include "gran/ui/tree_view.hpp"

#include <algorithm>
#include <cmath>

namespace gran::render::internal
{
    /*
    ** Кнопка раскрытия строки дерева: стрелка-галочка в середине кнопки — вправо у свёрнутой ветки, вниз у раскрытой
    ** (как в проводниках); у листа стрелки нет. Размах — треть меньшей стороны кнопки, цвет — цвет текста, приглушённый,
    ** как стрелка подменю.
    **
    ** The tree row expander button: a chevron in the middle of the button — right for a collapsed branch, down for an expanded one
    ** (as in file managers); a leaf has no arrow. The span is a third of the smaller button side, the color is the text color, muted,
    ** as the submenu arrow.
    */
    void paintExpander(const ui::Node& expander, const PaintState& state, SceneContext& context)
    {
        Rect content = contentRect(expander);
        float side = std::min(content.width, content.height);
        Chevron mark{Vec2{content.x + content.width * 0.5f, content.y + content.height * 0.5f}, Vec2{1.0f, 0.0f},
            std::round(side * 0.28f * 2.0f) * 0.5f, colorWithOpacity(expander.computed.color, 0.8f)};

        if (!ui::expanderHasBranch(expander) || side <= 0.0f)
            return;
        if (expander.content.checked)
            mark.direction = Vec2{0.0f, 1.0f};
        emitChevron(mark, state, context);
    }
}
