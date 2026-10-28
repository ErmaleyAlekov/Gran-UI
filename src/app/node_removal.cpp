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
        app::AppWindow* win = app::findWindow(*this, node);

        if (win == nullptr)
            return;
        app::forgetWebViews(*win, node);
        if (win->router)
            win->router->forget(node);
        forgetAnimations(win->animator, node);
        if (insideTree(win->access.focus, node))
            win->access.focus = nullptr;
        if (insideTree(win->pendingFocus, node))
            win->pendingFocus = nullptr;
        if (insideTree(win->dropNode, node))
            win->dropNode = nullptr;
    }
}
