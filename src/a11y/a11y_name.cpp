#include "gran/a11y/access_tree.hpp"
#include "gran/core/i18n.hpp"
#include <vector>

namespace gran::a11y
{
    namespace
    {
        // Имена команд берутся из каталога, а не из символа значка кнопки.
        // Command names come from the catalog rather than the button icon character.
        std::string windowButtonName(const ui::Node& node)
        {
            using platform::WindowRole;
            using i18n::Message;

            if (node.content.windowRole == WindowRole::MINIMIZE)
                return i18n::format(Message::WINDOW_MINIMIZE);
            if (node.content.windowRole == WindowRole::CLOSE)
                return i18n::format(Message::WINDOW_CLOSE);
            if ((node.stateMask & style::stateBit(style::State::MAXIMIZED)) != 0)
                return i18n::format(Message::WINDOW_RESTORE);
            return i18n::format(Message::WINDOW_MAXIMIZE);
        }

        // Обратный порядок сохраняет порядок чтения при извлечении из стека.
        // Reverse insertion preserves reading order when popping the stack.
        void pushChildren(const ui::Node& node, std::vector<const ui::Node*>& pending)
        {
            for (auto it = node.children.rbegin(); it != node.children.rend(); ++it)
                pending.push_back(it->get());
        }

        // Тексты потомков через пробел / Descendant texts separated by spaces
        void appendTexts(const ui::Node& node, std::string& out)
        {
            std::vector<const ui::Node*> pending;
            pushChildren(node, pending);
            while (!pending.empty())
            {
                const auto* child = pending.back();
                pending.pop_back();
                if (!child->visible || hiddenText(*child))
                    continue;
                if (!child->content.text.empty() && !out.empty())
                    out += ' ';
                out += child->content.text;
                pushChildren(*child, pending);
            }
        }

        // Имя из текстов детей: кнопка и части таблицы и дерева / The name from the children texts: a button and the table and tree parts
        bool namedByChildren(const ui::Node& node) noexcept
        {
            switch (roleOf(node))
            {
                case Role::BUTTON: case Role::HEADER_ITEM: case Role::DATA_ITEM: case Role::CELL: case Role::TREE_ITEM:
                    return true;
                default:
                    return false;
            }
        }

        /*
        ** Подпись из другого узла (labeledBy="id", как aria-labelledby): его текст или тексты его детей.
        ** Поиск идёт от корня дерева, в котором лежит узел; нет такого id — подписи нет.
        **
        ** A label from another node (labeledBy="id", as aria-labelledby): its text or its children texts.
        ** The search goes from the root of the tree holding the node; no such id — no label.
        */
        std::string labelText(const ui::Node& node)
        {
            const ui::Node* top = &node;
            std::string text;

            if (node.content.labeledBy.empty())
                return text;
            while (top->parent != nullptr)
                top = top->parent;
            std::vector<const ui::Node*> pending{top};
            while (!pending.empty())
            {
                const auto* label = pending.back();
                pending.pop_back();
                if (label->id != node.content.labeledBy)
                    pushChildren(*label, pending);
                else
                {
                    if (label == &node) break;
                    text = label->content.text;
                    if (text.empty()) appendTexts(*label, text);
                    break;
                }
            }
            return text;
        }
    }

    /*
    ** Имя как в HTML: явное accessibleName (aria-label), затем подпись labeledBy (aria-labelledby);
    ** у поля ввода и ComboBox — подсказка (placeholder; текст — это значение), у остальных — свой текст,
    ** у кнопки без текста — тексты детей; последний запасной вариант — tooltip.
    **
    ** The name as in HTML: the explicit accessibleName (aria-label), then the labeledBy label (aria-labelledby);
    ** for an input and a ComboBox — the placeholder (the text is the value), for the others — their own text,
    ** for a button without text — the children texts; the last fallback is the tooltip.
    */
    std::string accessibleName(const ui::Node& node)
    {
        std::string name = node.content.accessibleName;

        if (name.empty())
            name = labelText(node);
        if (!name.empty())
            return name;
        if (ui::isWindowButton(node))
            return windowButtonName(node);
        // Кнопка «ещё» панели рисует только стрелку: имя — из каталога / The toolbar "more" button draws only an arrow: the name is from the catalog
        if (node.kind == ui::ElementKind::TOOL_MORE)
            return i18n::format(i18n::Message::TOOLBAR_MORE);
        // Секция строки состояния читается целиком: «Строка 3, столбец 14» / A status section is read as a whole: "Line 3, column 14"
        if (node.kind == ui::ElementKind::STATUS_SECTION)
            return ui::statusText(node);
        if (ui::isTextField(node) || node.kind == ui::ElementKind::COMBO_BOX)
            name = node.content.placeholder;
        // Текст WebView — HTML страницы, а не имя: экранный диктор озвучивал бы разметку
        // The WebView text is the page HTML, not a name: a screen reader would speak the markup
        else if (node.kind != ui::ElementKind::WEB_VIEW)
            name = node.content.text;
        // Кнопка, заголовок столбца, строка и ячейка таблицы, строка дерева без своего текста — тексты детей
        // A button, a column header, a table row and cell, a tree row without own text — the children texts
        if (name.empty() && namedByChildren(node))
            appendTexts(node, name);
        if (name.empty())
            name = node.content.tooltip;
        return name;
    }
}
