#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <utility>

namespace gran
{
    /*
    ** Новый узел «грязный» целиком; markDirty поднимает DIRTY_DESCENDANT и DIRTY_LAYOUT
    ** к корню — проход стилей найдёт его, компоновка пересчитает ветвь. Списки items="…"
    ** в нём привязываются к моделям.
    **
    ** A new node is entirely "dirty"; markDirty raises DIRTY_DESCENDANT and DIRTY_LAYOUT
    ** to the root — the style pass finds it, the layout recomputes the branch. The items="…" lists
    ** in it are bound to the models.
    ** Подписи ComboBox с пунктами из данных ставятся после привязки / The labels of ComboBoxes with items from data are set after binding.
    */
    ui::Node& Application::append(ui::Node& parent, std::unique_ptr<ui::Node> child)
    {
        ui::Node& added = ui::appendChild(parent, std::move(child));

        ui::markDirty(added, ui::DIRTY_STYLE | ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        _state->lists.bind(added);
        ui::syncComboBoxes(added);
        return added;
    }

    // Корень удалить нельзя: у него нет родителя, который бы им владел / The root cannot be removed: it has no parent that would own it
    void Application::remove(ui::Node& node)
    {
        if (node.parent == nullptr)
            throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::APP_REMOVE_ROOT));
        _state->lists.removeNode(node);
    }

    // До run() маршрутизатора нет: фокус ставится при его создании / Before run() there is no router: the focus is set when it is created
    void Application::focus(ui::Node* node)
    {
        app::AppWindow* win = nullptr;

        if (_state->windows.empty())
            return;
        if (node != nullptr)
            win = &app::windowOf(*_state, *node);
        else
            win = _state->windows.front().get();
        if (win->router)
            win->router->focus(node);
        else
            win->pendingFocus = node;
    }
}
