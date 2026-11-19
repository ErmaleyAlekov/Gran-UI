#include "gran/ui/style_pass.hpp"
#include "gran/ui/data_grid.hpp"
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

        /*
        ** Стиль темы по виду узла, а для частей, у которых вид общий, — по месту: пункт строки меню, кнопка панели
        ** инструментов, закреплённый список строк таблицы (Stack-ребёнок DataGrid: до прокрутки — сверху, после — снизу),
        ** редактор ячейки таблицы (Input или ComboBox прямо в GridCell — GridEditor).
        **
        ** The theme style by the node kind, and for the parts with a shared kind — by the place: a menu bar item, a toolbar
        ** button, a table pinned row list (a Stack child of DataGrid: before the scroll — at the top, after it — at the bottom),
        ** a table cell editor (an Input or ComboBox right in a GridCell — GridEditor).
        */
        const style::StyleRule* defaultsFor(const Node& node, const KindStyles& defaults)
        {
            const Node* parent = node.parent;

            if (defaults.menuBarItem != nullptr && isMenuBarItem(node))
                return defaults.menuBarItem;
            if (defaults.toolBarButton != nullptr && node.kind == ElementKind::BUTTON && parent != nullptr
                && (parent->kind == ElementKind::TOOL_BAR || parent->kind == ElementKind::TOOL_GROUP))
                return defaults.toolBarButton;
            if (defaults.gridEditor != nullptr && parent != nullptr && parent->kind == ElementKind::GRID_CELL
                && (node.kind == ElementKind::INPUT || node.kind == ElementKind::COMBO_BOX))
                return defaults.gridEditor;
            if (node.kind == ElementKind::STACK && parent != nullptr && parent->kind == ElementKind::DATA_GRID)
            {
                const style::StyleRule* pinned = defaults.gridPinnedBottom;
                if (pinnedAbove(node))
                    pinned = defaults.gridPinnedTop;
                if (pinned != nullptr)
                    return pinned;
            }
            return defaults.rules[static_cast<std::size_t>(node.kind)];
        }

        // Новый вычисленный стиль узла; наблюдатель видит пересчёты и отдельно — первый расчёт
        // The new computed style of the node; the observer sees the recomputes and, separately, the first computation
        void recompute(Node& node, const style::ComputedStyle& parent, const StylePass& pass)
        {
            style::CascadeInput input;

            input.parent = &parent;
            input.defaults = defaultsFor(node, pass.defaults);
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
