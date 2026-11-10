#include "gran/ui/node.hpp"

namespace gran::ui
{
    namespace
    {
        /*
        ** Заголовок страницы: подпись, значение, подсказка и доступность страницы. Заголовок — отдельный
        ** узел, а не сама страница: так полоса раскладывается обычным стеком, а заголовок получает
        ** состояния Hover, Focused, Checked и стиль темы TabHeader.
        **
        ** A page header: the page label, value, tooltip and availability. The header is a separate
        ** node rather than the page itself: this way the strip is laid out as an ordinary stack, and the header gets
        ** the Hover, Focused, Checked states and the theme TabHeader style.
        */
        std::unique_ptr<Node> makeHeader(const Node& page)
        {
            std::unique_ptr<Node> header = std::make_unique<Node>();

            header->kind = ElementKind::TAB_HEADER;
            header->line = page.line;
            header->column = page.column;
            header->content.text = page.content.text;
            header->content.option = optionValue(page);
            header->content.tooltip = page.content.tooltip;
            header->content.accessibleName = page.content.accessibleName;
            header->enabled = page.enabled;
            return header;
        }

        // Полоса — первый ребёнок вида TabBar; нет — вставляется первой / The strip is the first child of the TabBar kind; none — it is inserted first
        Node& ensureBar(Node& tabs)
        {
            if (!tabs.children.empty() && tabs.children.front()->kind == ElementKind::TAB_BAR)
                return *tabs.children.front();
            std::unique_ptr<Node> bar = std::make_unique<Node>();

            bar->kind = ElementKind::TAB_BAR;
            bar->line = tabs.line;
            bar->column = tabs.column;
            return insertChild(tabs, 0, std::move(bar));
        }
    }

    /*
    ** Заголовки строятся заново, поэтому вызов — только при построении дерева (до run()): ввод
    ** может держать указатели на старые заголовки. Значение сбрасывается, чтобы selectTab
    ** расставил отметки и видимость страниц, даже если оно не изменилось.
    **
    ** The headers are built anew, so the call is only at tree build time (before run()): the input
    ** may hold pointers to the old headers. The value is reset so that selectTab
    ** places the marks and the page visibility even if it did not change.
    */
    void syncTabs(Node& tabs)
    {
        Node& bar = ensureBar(tabs);
        std::string value = tabs.content.option;

        bar.children.clear();
        for (const std::unique_ptr<Node>& child : tabs.children)
        {
            if (child->kind == ElementKind::TAB)
                appendChild(bar, makeHeader(*child));
        }
        // Меню вкладок при нехватке места — та же кнопка «ещё», что у панели инструментов / The tab menu when room is short — the same "more" button as the toolbar has
        if (tabs.content.tabs.overflowMenu)
            buildToolBarMore(bar);
        markDirty(bar, DIRTY_STYLE | DIRTY_LAYOUT | DIRTY_PAINT);
        tabs.content.option.clear();
        if (!selectTab(tabs, value) && !bar.children.empty())
            selectTab(tabs, optionValue(*bar.children.front()));
    }
}
