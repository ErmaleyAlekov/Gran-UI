#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <utility>

namespace gran
{
    namespace
    {
        // Анимации всех узлов поддерева снимаются (аниматор хранит указатели) / The animations of all subtree nodes are dropped (the animator keeps pointers)
        void forgetAnimations(animation::Animator& animator, const ui::Node& node)
        {
            animator.forget(node);
            for (const std::unique_ptr<ui::Node>& child : node.children)
                forgetAnimations(animator, *child);
        }

        // Лежит ли узел в поддереве / Whether the node lies in the subtree
        bool insideTree(const ui::Node* node, const ui::Node& subtree) noexcept
        {
            for (const ui::Node* current = node; current != nullptr; current = current->parent)
            {
                if (current == &subtree)
                    return true;
            }
            return false;
        }
    }

    /*
    ** Новый узел «грязный» целиком; markDirty поднимает DIRTY_DESCENDANT и DIRTY_LAYOUT
    ** к корню — проход стилей найдёт его, компоновка пересчитает ветвь.
    **
    ** A new node is entirely "dirty"; markDirty raises DIRTY_DESCENDANT and DIRTY_LAYOUT
    ** to the root — the style pass finds it, the layout recomputes the branch.
    */
    ui::Node& Application::append(ui::Node& parent, std::unique_ptr<ui::Node> child)
    {
        ui::Node& added = ui::appendChild(parent, std::move(child));

        ui::markDirty(added, ui::DIRTY_STYLE | ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        return added;
    }

    // Сначала снимаются все невладеющие ссылки, затем узел уничтожается / All non-owning references are dropped first, then the node is destroyed
    void Application::remove(ui::Node& node)
    {
        State& state = *_state;
        ui::Node* parent = node.parent;

        if (parent == nullptr)
            throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::APP_REMOVE_ROOT));
        if (state.router)
            state.router->forget(node);
        forgetAnimations(state.animator, node);
        if (insideTree(state.access.focus, node))
            state.access.focus = nullptr;
        if (insideTree(state.pendingFocus, node))
            state.pendingFocus = nullptr;
        std::vector<std::unique_ptr<ui::Node>>& siblings = parent->children;
        for (std::size_t index = 0; index < siblings.size(); ++index)
        {
            if (siblings[index].get() == &node)
            {
                siblings.erase(siblings.begin() + static_cast<std::ptrdiff_t>(index));
                break;
            }
        }
        ui::markDirty(*parent, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
    }

    // До run() маршрутизатора нет: фокус ставится при его создании / Before run() there is no router: the focus is set when it is created
    void Application::focus(ui::Node* node)
    {
        State& state = *_state;

        if (state.router)
            state.router->focus(node);
        else
            state.pendingFocus = node;
    }
}
