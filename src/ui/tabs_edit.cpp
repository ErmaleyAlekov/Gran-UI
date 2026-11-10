#include "gran/ui/node.hpp"

namespace gran::ui
{
    namespace
    {
        // Номер ребёнка-страницы Tab с номером page среди страниц (размер детей — нет такой) / The child number of the Tab page number page among the pages (the children size — none)
        std::size_t pageChild(const Node& tabs, std::size_t page) noexcept
        {
            std::size_t seen = 0;

            for (std::size_t index = 0; index < tabs.children.size(); ++index)
            {
                if (tabs.children[index]->kind != ElementKind::TAB)
                    continue;
                if (seen == page)
                    return index;
                ++seen;
            }
            return tabs.children.size();
        }

        // Переставить ребёнка from на место to (сдвигая соседей) / Moving the child from to the place to (shifting the neighbors)
        void moveChild(Node& parent, std::size_t from, std::size_t to)
        {
            std::unique_ptr<Node> moved = std::move(parent.children[from]);

            parent.children.erase(parent.children.begin() + static_cast<std::ptrdiff_t>(from));
            parent.children.insert(parent.children.begin() + static_cast<std::ptrdiff_t>(to), std::move(moved));
        }
    }

    // Номер заголовка в полосе = номер страницы среди детей Tab / The header number in the strip = the page number among the Tab children
    std::size_t headerNumber(const Node& header) noexcept
    {
        const Node* bar = header.parent;

        for (std::size_t index = 0; bar != nullptr && index < bar->children.size(); ++index)
        {
            if (bar->children[index].get() == &header)
                return index;
        }
        return 0;
    }

    // Страница заголовка: та же по счёту среди детей Tab / The header page: the same by count among the Tab children
    Node* pageOf(Node& header) noexcept
    {
        Node* tabs = tabsOf(header);
        std::size_t child = 0;

        if (tabs == nullptr)
            return nullptr;
        child = pageChild(*tabs, headerNumber(header));
        if (child >= tabs->children.size())
            return nullptr;
        return tabs->children[child].get();
    }

    /*
    ** Перестановка вкладки: заголовок и его страница сдвигаются вместе (номера заголовков и страниц совпадают).
    ** Узлы не создаются и не удаляются — указатели ввода и анимаций остаются верными. Порядок в разметке (DOM)
    ** не меняется: его хранит программа (onReorder), если нужно. true — вкладка сдвинулась.
    **
    ** Reordering a tab: the header and its page move together (the header and page numbers match).
    ** No nodes are created or removed — the input and animation pointers stay valid. The markup (DOM) order
    ** does not change: the program keeps it (onReorder) if needed. true — the tab moved.
    */
    bool moveTab(Node& tabs, std::size_t from, std::size_t to)
    {
        if (tabs.kind != ElementKind::TABS || tabs.children.empty() || tabs.children.front()->kind != ElementKind::TAB_BAR)
            return false;
        Node& bar = *tabs.children.front();
        std::size_t pages = 0;
        // Считаются только заголовки: в конце полосы может стоять кнопка меню вкладок / Only the headers count: the tab menu button may stand at the strip end
        while (pages < bar.children.size() && bar.children[pages]->kind == ElementKind::TAB_HEADER)
            ++pages;
        if (from == to || from >= pages || to >= pages)
            return false;
        std::size_t source = pageChild(tabs, from);
        std::size_t target = pageChild(tabs, to);
        if (source >= tabs.children.size() || target >= tabs.children.size())
            return false;
        moveChild(bar, from, to);
        moveChild(tabs, source, target);
        markDirty(tabs, DIRTY_LAYOUT | DIRTY_PAINT);
        return true;
    }
}
