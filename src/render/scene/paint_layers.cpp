#include "gran/render/internal/scene_internal.hpp"

#include <cmath>

namespace gran::render::internal
{
    namespace
    {
        // Размах стрелки в кеглях текста узла / The arrow span in the node font sizes
        constexpr float MARK_SIZE = 0.28f;

        /*
        ** Затемнение под модальным диалогом: прямоугольник на всё окно цветом scrim, умноженным
        ** на прозрачность диалога (затемнение появляется и исчезает вместе с ним).
        **
        ** The dimming under a modal dialog: a rectangle over the whole window in the scrim color multiplied
        ** by the dialog opacity (the dimming appears and disappears together with it).
        */
        void paintScrim(const ui::Node& dialog, const PaintState& base, SceneContext& context)
        {
            Color color = dialog.computed.scrim;

            color.a *= dialog.computed.opacity * base.opacity;
            if (color.a <= 0.0f)
                return;
            emitBox(ControlBox{context.viewport, 0.0f, color, color, 0.0f}, 0.0f, base, context);
        }
    }

    /*
    ** Открытые слои поверх всего дерева в порядке дерева: без отсечений и преобразований предков
    ** (слой живёт в координатах окна), со своим преобразованием и прозрачностью. Слои внутри слоя
    ** (подменю, список ComboBox, меню в диалоге) рисуются сразу после него — над ним.
    **
    ** The open layers over the whole tree in tree order: without the ancestor clips and transforms
    ** (a layer lives in window coordinates), with its own transform and opacity. The layers inside a layer
    ** (a submenu, a ComboBox list, a menu in a dialog) are painted right after it — over it.
    */
    void paintPopups(const ui::Node& node, const PaintState& base, SceneContext& context)
    {
        for (const std::unique_ptr<ui::Node>& child : node.children)
        {
            if (!child->visible)
                continue;
            if (ui::isLayer(*child))
            {
                if (child->kind == ui::ElementKind::DIALOG)
                    paintScrim(*child, base, context);
                context.layer = child.get();
                paintNode(*child, base, context);
                context.layer = nullptr;
            }
            paintPopups(*child, base, context);
        }
    }

    // Стрелка — в середине правого отступа (ComboBox) или у правого края содержимого (пункт с подменю); у пункта без подменю — подпись сочетания
    // The arrow is in the middle of the right padding (a ComboBox) or at the right content edge (an item with a submenu); an item without a submenu gets the shortcut label
    void paintMark(const ui::Node& node, const PaintState& state, SceneContext& context)
    {
        const style::ComputedStyle& style = node.computed;
        Rect content = contentRect(node);
        float size = std::round(style.fontSize * MARK_SIZE * 2.0f) * 0.5f;
        Chevron mark{Vec2{content.x + content.width - size, content.y + content.height * 0.5f}, Vec2{1.0f, 0.0f},
            size, colorWithOpacity(style.color, 0.65f)};

        if (node.kind == ui::ElementKind::MENU_ITEM && (ui::submenuOf(node) == nullptr || ui::isMenuBarItem(node)))
        {
            paintShortcut(node, state, context);
            return;
        }
        if (node.kind == ui::ElementKind::COMBO_BOX)
        {
            mark.center.x = content.x + content.width + style.padding.right * 0.5f;
            mark.direction = Vec2{0.0f, 1.0f};
        }
        // Кнопка «ещё» панели: стрелка вниз по центру / The toolbar "more" button: a down arrow in the center
        if (node.kind == ui::ElementKind::TOOL_MORE)
        {
            mark.center.x = content.x + content.width * 0.5f;
            mark.direction = Vec2{0.0f, 1.0f};
        }
        emitChevron(mark, state, context);
    }
}
