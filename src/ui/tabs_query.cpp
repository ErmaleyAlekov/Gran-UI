#include "gran/ui/node.hpp"

namespace gran::ui
{
    // Заголовок с тем же номером, что у страницы среди детей Tab / The header with the same number as the page among the Tab children
    Node* headerOfPage(Node& page) noexcept
    {
        Node* tabs = page.parent;
        std::size_t number = 0;

        if (page.kind != ElementKind::TAB || tabs == nullptr || tabs->kind != ElementKind::TABS || tabs->children.empty()
            || tabs->children.front()->kind != ElementKind::TAB_BAR)
            return nullptr;
        for (const std::unique_ptr<Node>& child : tabs->children)
        {
            if (child.get() == &page)
                break;
            if (child->kind == ElementKind::TAB)
                ++number;
        }
        Node& bar = *tabs->children.front();
        if (number >= bar.children.size() || bar.children[number]->kind != ElementKind::TAB_HEADER)
            return nullptr;
        return bar.children[number].get();
    }

    // Атрибут closable у Tabs над полосой заголовка / The closable attribute of the Tabs above the header strip
    bool tabClosable(const Node& header) noexcept
    {
        const Node* bar = header.parent;

        return header.kind == ElementKind::TAB_HEADER && bar != nullptr && bar->parent != nullptr
            && bar->parent->content.tabs.closable;
    }

    /*
    ** Закрытая вкладка была выбранной — отметки ни на одном заголовке нет: выбирается последняя из оставшихся
    ** (как в редакторах после закрытия крайней справа) — значение value Tabs меняется без onChange (это код).
    **
    ** The closed tab was the selected one — no header has the mark: the last of the remaining ones is chosen
    ** (as in editors after closing the rightmost one) — the Tabs value changes without onChange (this is code).
    */
    void repairTabSelection(Node& tabs)
    {
        Node* last = nullptr;

        if (tabs.kind != ElementKind::TABS || tabs.children.empty() || tabs.children.front()->kind != ElementKind::TAB_BAR)
            return;
        for (const std::unique_ptr<Node>& header : tabs.children.front()->children)
        {
            if (header->kind != ElementKind::TAB_HEADER)
                continue;
            if (header->content.checked)
                return;
            last = header.get();
        }
        tabs.content.option.clear();
        if (last != nullptr)
            selectTab(tabs, optionValue(*last));
    }
}
