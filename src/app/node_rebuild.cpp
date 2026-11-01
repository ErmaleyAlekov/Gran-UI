#include "gran/app/internal/app_state.hpp"
#include "gran/markup/live_document.hpp"
#include "gran/ui/node_state.hpp"

#include <utility>

namespace gran::app
{
    namespace
    {
        /*
        ** Дети, добавленные во время работы (подсказка и контекстное меню окна у корня, узлы из кода),
        ** вынимаются до снятия ссылок: их привязки, ссылки ввода и указатели окна остаются действительными.
        **
        ** The children added at run time (the window tooltip and context menu at the root, the nodes from code)
        ** are taken out before the references are dropped: their bindings, input references and window pointers stay valid.
        */
        std::vector<std::unique_ptr<ui::Node>> takeRuntimeChildren(ui::Node& node)
        {
            std::vector<std::unique_ptr<ui::Node>> kept;

            for (std::size_t index = node.children.size(); index > 0; --index)
            {
                if (!markup::isRuntimeChild(node, *node.children[index - 1]))
                    continue;
                kept.insert(kept.begin(), std::move(node.children[index - 1]));
                kept.front()->parent = nullptr;
                node.children.erase(node.children.begin() + static_cast<std::ptrdiff_t>(index - 1));
            }
            return kept;
        }

        /*
        ** Прежние потомки уходят так же, как при remove: привязки списков (и самого узла — он привяжется
        ** заново), ссылки ввода, анимации, доступности и встроенных браузеров. Анимации самого узла
        ** снимаются: его стиль строится заново.
        **
        ** The former descendants go the same way as on remove: the list bindings (the node own too — it is bound
        ** again), the input, animation, accessibility and embedded browser references. The node own animations
        ** are dropped: its style is built anew.
        */
        void dropDescendants(AppWindow& win, ui::Node& node)
        {
            win.shared.lists.forget(node);
            win.animator.forget(node);
            for (const std::unique_ptr<ui::Node>& child : node.children)
                win.shared.nodeRemoving(*child);
        }

        /*
        ** Перенос содержимого: адрес, родитель, слушатели и состояния ввода (наведение, нажатие, фокус…)
        ** остаются, дети узнают нового родителя.
        **
        ** Moving the content: the address, the parent, the listeners and the input states (hover, press, focus…)
        ** stay, the children learn their new parent.
        */
        void adoptContent(ui::Node& node, ui::Node&& fresh)
        {
            ui::Node* parent = node.parent;
            std::vector<ui::EventListener> listeners = std::move(node.listeners);
            std::uint16_t markupStates = static_cast<std::uint16_t>(style::stateBit(style::State::DISABLED) | style::stateBit(style::State::CHECKED));
            std::uint16_t kept = static_cast<std::uint16_t>(node.stateMask & ~markupStates);

            node = std::move(fresh);
            node.parent = parent;
            node.listeners = std::move(listeners);
            node.stateMask = static_cast<std::uint16_t>(node.stateMask | kept);
            for (const std::unique_ptr<ui::Node>& child : node.children)
                child->parent = &node;
        }

        /*
        ** Новое поддерево готовится как при append; родитель, собранный из детей (полоса заголовков
        ** Tabs, слой пунктов ComboBox), собирается заново — прежние заголовки снимаются со ссылок ввода.
        **
        ** The new subtree is prepared as on append; a parent assembled from its children (the Tabs header
        ** strip, the ComboBox item layer) is assembled anew — the former headers are dropped from the input references.
        */
        void finishSubtree(AppWindow& win, ui::Node& node)
        {
            ui::Node* parent = node.parent;

            ui::applyInitialStates(node);
            ui::markDirty(node, ui::DIRTY_STYLE | ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
            bindCommands(win, node);
            win.shared.lists.bind(node);
            ui::syncComboBoxes(node);
            if (parent != nullptr && parent->kind == ui::ElementKind::TABS && !parent->children.empty())
            {
                for (const std::unique_ptr<ui::Node>& header : parent->children.front()->children)
                    win.shared.nodeRemoving(*header);
                ui::syncTabs(*parent);
            }
            if (parent != nullptr && parent->parent != nullptr && parent->parent->kind == ui::ElementKind::COMBO_BOX)
                ui::syncComboBoxes(*parent->parent);
        }
    }

    // Фокус ставится заново, только если он был у потомка: узел в фокусе router уже помнит
    // The focus is set again only if a descendant had it: the router already remembers a focused node
    void rebuildInPlace(AppWindow& win, ui::Node& node, std::unique_ptr<ui::Node> fresh, bool keepOwnState)
    {
        const ui::Node* focused = nullptr;

        if (win.router)
            focused = win.router->focused();
        ui::TreeState saved = ui::captureState(node, focused);
        std::vector<std::unique_ptr<ui::Node>> runtime = takeRuntimeChildren(node);
        dropDescendants(win, node);
        adoptContent(node, std::move(*fresh));
        finishSubtree(win, node);
        for (std::unique_ptr<ui::Node>& child : runtime)
            ui::appendChild(node, std::move(child));
        ui::Node* focus = ui::restoreState(node, saved, !keepOwnState);
        if (focus != nullptr && win.router && win.router->focused() != focus)
            win.router->focus(focus);
        win.changed = true;
    }
}
