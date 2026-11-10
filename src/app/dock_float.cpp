#include "gran/app/internal/app_state.hpp"

#include <utility>
#include <vector>

namespace gran::app
{
    namespace
    {
        /*
        ** Перенос детей from в конец to теми же узлами. Перед переносом окно-источник снимает свои ссылки
        ** (наведение, фокус, анимации, доступность) — как при удалении, но узлы не уничтожаются; привязки списков
        ** и выражений общие для всех окон и остаются верными. Стиль поддерева пересчитывается в новом месте.
        **
        ** Moving the children of from to the end of to as the same nodes. Before the move the source window drops its references
        ** (hover, focus, animations, accessibility) — as on removal, but the nodes are not destroyed; the list and expression
        ** bindings are shared by all the windows and stay valid. The subtree style is recomputed at the new place.
        */
        void moveChildren(Application::State& state, ui::Node& from, ui::Node& to)
        {
            std::vector<std::unique_ptr<ui::Node>> moved;

            for (const std::unique_ptr<ui::Node>& child : from.children)
                state.nodeRemoving(*child);
            moved.swap(from.children);
            ui::markDirty(from, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
            for (std::unique_ptr<ui::Node>& child : moved)
            {
                ui::Node& placed = ui::appendChild(to, std::move(child));
                ui::markDirty(placed, ui::DIRTY_STYLE | ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
            }
        }

        // Содержимое окна-панели — в корень его документа (это Stack из разметки окна, а не сам Window) / The panel window content goes into its document root (the Stack from the window markup, not the Window itself)
        ui::Node* floatTarget(AppWindow& floatWin) noexcept
        {
            return floatWin.document.root.get();
        }
    }

    // Окно запоминает панель: по ней содержимое вернётся при закрытии окна / The window remembers the panel: by it the content returns when the window closes
    void floatPanel(Application::State& state, ui::Node& panel, AppWindow& floatWin)
    {
        ui::Node* target = floatTarget(floatWin);

        if (target == nullptr || panel.content.dock.floating)
            return;
        moveChildren(state, panel, *target);
        panel.content.dock.floating = true;
        floatWin.floatingPanel = &panel;
        if (panel.parent != nullptr)
            ui::markDirty(*panel.parent, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
    }

    // Содержимое возвращается до того, как окно снимет привязки и уничтожит своё дерево / The content returns before the window drops the bindings and destroys its tree
    void returnPanel(Application::State& state, AppWindow& floatWin)
    {
        ui::Node* panel = floatWin.floatingPanel;
        ui::Node* target = floatTarget(floatWin);

        floatWin.floatingPanel = nullptr;
        if (panel == nullptr || target == nullptr)
            return;
        moveChildren(state, *target, *panel);
        panel->content.dock.floating = false;
        ui::chooseDockPanel(*panel);
    }

    // Окон немного: линейный поиск / There are few windows: a linear search
    AppWindow* floatWindowOf(Application::State& state, const ui::Node& panel) noexcept
    {
        for (const std::unique_ptr<AppWindow>& win : state.windows)
        {
            if (win->floatingPanel == &panel)
                return win.get();
        }
        return nullptr;
    }
}
