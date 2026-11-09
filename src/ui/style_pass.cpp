#include "gran/ui/style_pass.hpp"
#include "gran/style/style_compare.hpp"

#include <utility>

namespace gran::ui
{
    namespace
    {
        // Параметры прохода / The pass parameters
        struct StylePass
        {
            const KindStyles& defaults;
            StyleObserver* observer;
        };

        // Новый вычисленный стиль узла; наблюдатель видит пересчёты и отдельно — первый расчёт
        // The new computed style of the node; the observer sees the recomputes and, separately, the first computation
        void recompute(Node& node, const style::ComputedStyle& parent, const StylePass& pass)
        {
            style::CascadeInput input;

            input.parent = &parent;
            input.defaults = pass.defaults.rules[static_cast<std::size_t>(node.kind)];
            if (pass.defaults.menuBarItem != nullptr && isMenuBarItem(node))
                input.defaults = pass.defaults.menuBarItem;
            if (pass.defaults.toolBarButton != nullptr && node.kind == ElementKind::BUTTON && node.parent != nullptr
                && (node.parent->kind == ElementKind::TOOL_BAR || node.parent->kind == ElementKind::TOOL_GROUP))
                input.defaults = pass.defaults.toolBarButton;
            input.rule = node.rule;
            input.inlineBlock = &node.declared;
            input.stateMask = node.stateMask;
            style::ComputedStyle next = style::computeStyle(input);
            if (node.styled && pass.observer != nullptr)
                pass.observer->styleChanged(node, node.computed, next);
            else if (pass.observer != nullptr)
                pass.observer->styleCreated(node, next);
            node.styled = true;
            if (style::affectsLayout(node.computed, next))
                markDirty(node, DIRTY_LAYOUT);
            node.computed = std::move(next);
            node.dirty = static_cast<std::uint8_t>(node.dirty | DIRTY_PAINT);
        }

        // Узел и его поддерево; force — родитель пересчитан, наследуемые свойства могли смениться
        // The node and its subtree; force — the parent is recomputed, the inherited properties could change
        void resolveNode(Node& node, const style::ComputedStyle& parent, const StylePass& pass, bool force)
        {
            bool self = force || (node.dirty & DIRTY_STYLE) != 0;

            if (!self && (node.dirty & DIRTY_DESCENDANT) == 0)
                return;
            if (self)
                recompute(node, parent, pass);
            for (const std::unique_ptr<Node>& child : node.children)
                resolveNode(*child, node.computed, pass, self);
            // Флаги снимаются после детей: их пометки поднимают DIRTY_DESCENDANT обратно
            // The flags are cleared after the children: their marks raise DIRTY_DESCENDANT back
            node.dirty = static_cast<std::uint8_t>(node.dirty & ~(DIRTY_STYLE | DIRTY_DESCENDANT));
        }
    }

    // Корень наследует от окна / The root inherits from the window
    void resolveStyles(Node& root, const style::ComputedStyle& windowStyle, const KindStyles& defaults,
        StyleObserver* observer)
    {
        StylePass pass{defaults, observer};

        resolveNode(root, windowStyle, pass, false);
    }
}
