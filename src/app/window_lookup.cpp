#include "gran/app/internal/app_state.hpp"

namespace gran::app
{
    // Корень дерева узла — корень документа одного из окон / The node tree root is the document root of one of the windows
    AppWindow* findWindow(Application::State& state, const ui::Node& node) noexcept
    {
        const ui::Node* root = &node;

        while (root->parent != nullptr)
            root = root->parent;
        for (const std::unique_ptr<AppWindow>& win : state.windows)
        {
            if (win->document.root.get() == root)
                return win.get();
        }
        return nullptr;
    }

    // Маршрутизатор есть только у показанного окна / Only a shown window has a router
    input::InputRouter* routerOf(Application::State& state, const ui::Node& node) noexcept
    {
        AppWindow* win = findWindow(state, node);

        if (win == nullptr)
            return nullptr;
        return win->router.get();
    }
}
