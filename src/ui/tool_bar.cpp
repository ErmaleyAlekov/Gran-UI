#include "gran/ui/node.hpp"

namespace gran::ui
{
    namespace
    {
        // Подпись пункта-зеркала: текст кнопки, без него — подсказка или имя для дикторов (кнопка-значок)
        // The mirror item label: the button text, without it — the tooltip or the screen reader name (an icon button)
        const std::string& overflowLabel(const Node& source) noexcept
        {
            if (!source.content.text.empty())
                return source.content.text;
            if (!source.content.tooltip.empty())
                return source.content.tooltip;
            return source.content.accessibleName;
        }

        /*
        ** Узел панели → пункты-зеркала меню «ещё»: разделитель — разделитель, группа — зеркала её детей по порядку,
        ** прочее — пункт с той же подписью, командой, onClick и значением (у заголовка вкладки — значение страницы). Подпись, включённость и сочетание команды пункт
        ** получает при привязке команд — так же, как кнопка, и меняется вместе с ней.
        **
        ** A toolbar node → the "more" menu mirror items: a separator — a separator, a group — the mirrors of its children in order,
        ** the rest — an item with the same label, command, onClick and value (for a tab header — the page value). The command label, enabled state and shortcut come to the item
        ** at the command binding — the same way as to the button, and change together with it.
        */
        void appendMirror(Node& menu, const Node& source)
        {
            if (source.kind == ElementKind::TOOL_GROUP)
            {
                for (const std::unique_ptr<Node>& child : source.children)
                    appendMirror(menu, *child);
                return;
            }
            std::unique_ptr<Node> item = std::make_unique<Node>();
            item->line = source.line;
            item->column = source.column;
            item->visible = false;
            item->kind = ElementKind::MENU_ITEM;
            if (source.kind == ElementKind::SEPARATOR)
                item->kind = ElementKind::SEPARATOR;
            item->content.text = overflowLabel(source);
            item->content.command = source.content.command;
            item->content.onClick = source.content.onClick;
            item->content.option = source.content.option;
            setEnabled(*item, source.enabled);
            appendChild(menu, std::move(item));
        }
    }

    // Кнопка «ещё» — последний ребёнок панели или полосы вкладок с overflowMenu / The "more" button is the last child of a toolbar or of a tab strip with overflowMenu
    Node* toolBarMore(Node& bar) noexcept
    {
        if ((bar.kind != ElementKind::TOOL_BAR && bar.kind != ElementKind::TAB_BAR) || bar.children.empty()
            || bar.children.back()->kind != ElementKind::TOOL_MORE)
            return nullptr;
        return bar.children.back().get();
    }

    /*
    ** Кнопка «ещё» строится движком (как полоса заголовков Tabs): скрыта, пока всё помещается; её единственный
    ** ребёнок — слой меню с зеркалом каждого ребёнка панели. Видимость зеркал — по ушедшим детям (syncOverflowMenu):
    ** во время работы узлы не создаются и не удаляются.
    **
    ** The "more" button is built by the engine (as the Tabs header strip): hidden while everything fits; its only
    ** child is the menu layer with a mirror of every toolbar child. The mirror visibility follows the children that went (syncOverflowMenu):
    ** no nodes are created or removed at run time.
    */
    void buildToolBarMore(Node& bar)
    {
        std::unique_ptr<Node> more = std::make_unique<Node>();
        std::unique_ptr<Node> menu = std::make_unique<Node>();

        more->kind = ElementKind::TOOL_MORE;
        more->visible = false;
        more->line = bar.line;
        more->column = bar.column;
        menu->kind = ElementKind::POPUP;
        menu->visible = false;
        for (const std::unique_ptr<Node>& child : bar.children)
            appendMirror(*menu, *child);
        appendChild(*more, std::move(menu));
        appendChild(bar, std::move(more));
    }
}
