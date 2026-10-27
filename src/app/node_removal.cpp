#include "gran/app/internal/app_state.hpp"

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

    // Привязки списков снимает ListBindings::removeNode, узел уничтожает он же
    // ListBindings::removeNode drops the list bindings and destroys the node itself
    void Application::State::nodeRemoving(const ui::Node& node) noexcept
    {
        app::forgetWebViews(*this, node);
        if (router)
            router->forget(node);
        forgetAnimations(animator, node);
        if (insideTree(access.focus, node))
            access.focus = nullptr;
        if (insideTree(pendingFocus, node))
            pendingFocus = nullptr;
    }
}
