#include "gran/ui/node.hpp"

namespace gran::ui
{
    namespace
    {
        /*
        ** Видна только страница с номером index (номер заголовка в полосе = номер страницы среди детей Tab).
        ** Смена видимости меняет компоновку Tabs: markDirty поднимает DIRTY_LAYOUT к предкам.
        **
        ** Only the page with the number index is visible (the header number in the strip = the page number among the Tab children).
        ** A visibility change changes the Tabs layout: markDirty raises DIRTY_LAYOUT to the ancestors.
        */
        void showPage(Node& tabs, std::size_t index)
        {
            std::size_t page = 0;

            for (const std::unique_ptr<Node>& child : tabs.children)
            {
                if (child->kind != ElementKind::TAB)
                    continue;
                bool shown = page == index;
                ++page;
                if (child->visible == shown)
                    continue;
                child->visible = shown;
                markDirty(*child, DIRTY_STYLE | DIRTY_LAYOUT | DIRTY_PAINT);
            }
        }

        // Номер заголовка со значением value в полосе (размер полосы — нет такого) / The number of the header with the value value in the strip (the strip size — none)
        std::size_t headerIndex(const Node& bar, const std::string& value) noexcept
        {
            std::size_t index = 0;

            while (index < bar.children.size() && optionValue(*bar.children[index]) != value)
                ++index;
            return index;
        }
    }

    // Нет такой страницы — выбор не меняется / No such page — the choice does not change
    bool selectTab(Node& tabs, const std::string& value)
    {
        if (tabs.kind != ElementKind::TABS || tabs.children.empty() || tabs.children.front()->kind != ElementKind::TAB_BAR)
            return false;
        Node& bar = *tabs.children.front();
        std::size_t index = headerIndex(bar, value);

        if (index == bar.children.size())
            return false;
        for (std::size_t header = 0; header < bar.children.size(); ++header)
            setChecked(*bar.children[header], header == index);
        showPage(tabs, index);
        bool changed = tabs.content.option != value;
        tabs.content.option = value;
        return changed;
    }

    // Заголовок — ребёнок полосы, полоса — ребёнок Tabs / A header is a strip child, the strip is a Tabs child
    Node* tabsOf(Node& header) noexcept
    {
        if (header.kind != ElementKind::TAB_HEADER || header.parent == nullptr || header.parent->parent == nullptr)
            return nullptr;
        return header.parent->parent;
    }
}
