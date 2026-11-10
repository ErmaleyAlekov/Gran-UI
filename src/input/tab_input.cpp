#include "gran/input/input_router.hpp"

namespace gran::input
{
    namespace
    {
        // Ближайшие Tabs: у заголовка — его полоса, иначе предок узла фокуса (страница) / The nearest Tabs: for a header — its strip, otherwise an ancestor of the focus node (a page)
        ui::Node* tabsAround(ui::Node* node) noexcept
        {
            if (node != nullptr && node->kind == ui::ElementKind::TAB_HEADER)
                return ui::tabsOf(*node);
            for (ui::Node* current = node; current != nullptr; current = current->parent)
            {
                if (current->kind == ui::ElementKind::TABS)
                    return current;
            }
            return nullptr;
        }

        // Заголовок выбранной вкладки (отметка Checked) / The header of the selected tab (the Checked mark)
        ui::Node* selectedHeader(ui::Node& tabs) noexcept
        {
            if (tabs.children.empty() || tabs.children.front()->kind != ui::ElementKind::TAB_BAR)
                return nullptr;
            for (const std::unique_ptr<ui::Node>& header : tabs.children.front()->children)
            {
                if (header->kind == ui::ElementKind::TAB_HEADER && header->content.checked)
                    return header.get();
            }
            return nullptr;
        }
    }

    // Запрос закрыть вкладку: onClose у Tabs со страницей; закрывает сама программа (app.remove) / A request to close a tab: onClose of Tabs with the page; the program closes it itself (app.remove)
    void requestTabClose(RouterState& state, ui::Node& header)
    {
        ui::Node* tabs = ui::tabsOf(header);
        ui::Node* page = ui::pageOf(header);

        if (tabs != nullptr && page != nullptr && tabs->content.tabs.closable && !tabs->content.tabs.onClose.empty())
            state.actions->action(tabs->content.tabs.onClose, *page);
    }

    /*
    ** Клавиши вкладок документов в фокусе внутри Tabs: Ctrl+Tab и Ctrl+Shift+Tab — следующая и предыдущая вкладка
    ** по кругу (выбор и onChange, как стрелки), Ctrl+W и Ctrl+F4 — закрыть выбранную (если closable). true — обработано.
    **
    ** The document tab keys with the focus inside Tabs: Ctrl+Tab and Ctrl+Shift+Tab — the next and the previous tab
    ** cyclically (the choice and onChange, as the arrows), Ctrl+W and Ctrl+F4 — close the selected one (if closable). true — handled.
    */
    bool tabsKey(RouterState& state, const platform::Event& event)
    {
        ui::Node* tabs = tabsAround(state.focus);
        bool shift = (event.modifiers & platform::MODIFIER_SHIFT) != 0;

        if (tabs == nullptr || (event.modifiers & platform::MODIFIER_CONTROL) == 0)
            return false;
        ui::Node* current = selectedHeader(*tabs);
        if (current == nullptr)
            return false;
        if ((event.key == platform::Key::W || event.key == platform::Key::F4) && tabs->content.tabs.closable)
        {
            requestTabClose(state, *current);
            return true;
        }
        if (event.key != platform::Key::TAB)
            return false;
        std::vector<ui::Node*> group = ui::choiceGroup(*current);
        std::size_t index = 0;
        while (index < group.size() && group[index] != current)
            ++index;
        if (group.empty() || index == group.size())
            return true;
        std::size_t next = (index + 1) % group.size();
        if (shift)
            next = (index + group.size() - 1) % group.size();
        changeChoice(state, *group[next]);
        return true;
    }

    // Пункт меню вкладок (зеркало заголовка в кнопке «ещё» полосы) выбирает свою вкладку; false — не такой пункт
    // An item of the tab menu (a header mirror in the strip "more" button) chooses its tab; false — not such an item
    bool chooseTabMirror(RouterState& state, ui::Node& item)
    {
        ui::Node* menu = item.parent;
        ui::Node* more = nullptr;

        if (menu != nullptr)
            more = menu->parent;
        if (more == nullptr || more->kind != ui::ElementKind::TOOL_MORE || more->parent == nullptr
            || more->parent->kind != ui::ElementKind::TAB_BAR)
            return false;
        for (const std::unique_ptr<ui::Node>& header : more->parent->children)
        {
            if (header->kind == ui::ElementKind::TAB_HEADER && header->content.option == item.content.option)
            {
                changeChoice(state, *header);
                return true;
            }
        }
        return true;
    }
}
