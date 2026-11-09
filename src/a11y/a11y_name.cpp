#include "gran/a11y/access_tree.hpp"
#include "gran/core/i18n.hpp"

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

        // Тексты потомков через пробел (подпись кнопки из детей: значок + Text) / The descendant texts separated by spaces (a button label from its children: an icon + a Text)
        void appendTexts(const ui::Node& node, std::string& out)
        {
            for (const std::unique_ptr<ui::Node>& child : node.children)
            {
                if (!child->visible)
                    continue;
                if (!child->content.text.empty() && !out.empty())
                    out += ' ';
                out += child->content.text;
                appendTexts(*child, out);
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
            // findNode только читает дерево; неконстантная сигнатура — ради вызывающих, которые меняют найденное
            // findNode only reads the tree; the non-const signature is for the callers that change the found node
            const ui::Node* label = ui::findNode(const_cast<ui::Node&>(*top), node.content.labeledBy);
            if (label == nullptr || label == &node)
                return text;
            text = label->content.text;
            if (text.empty())
                appendTexts(*label, text);
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
        if (ui::isTextField(node) || node.kind == ui::ElementKind::COMBO_BOX)
            name = node.content.placeholder;
        // Текст WebView — HTML страницы, а не имя: экранный диктор озвучивал бы разметку
        // The WebView text is the page HTML, not a name: a screen reader would speak the markup
        else if (node.kind != ui::ElementKind::WEB_VIEW)
            name = node.content.text;
        if (name.empty() && node.kind == ui::ElementKind::BUTTON)
            appendTexts(node, name);
        if (name.empty())
            name = node.content.tooltip;
        return name;
    }
}
