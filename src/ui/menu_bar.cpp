#include "gran/ui/node.hpp"
#include "gran/data/record_order.hpp"
#include "gran/utils/utf8.hpp"

#include <vector>

namespace gran::ui
{
    /*
    ** Подпись копируется без знаков «_»: «_X» даёт букву X (берётся первая такая), «__» — сам знак «_»,
    ** одиночный «_» в конце подписи остаётся как есть. Смещение буквы — в байтах уже очищенной подписи.
    **
    ** The label is copied without the "_" signs: "_X" gives the letter X (the first such one is taken), "__" is the "_" sign
    ** itself, a single "_" at the label end stays as is. The letter offset is in bytes of the already cleaned label.
    */
    bool takeMnemonic(Node& item)
    {
        const std::string& text = item.content.text;
        std::string label;
        std::size_t position = 0;

        item.content.mnemonic = 0;
        if (text.find('_') == std::string::npos)
            return false;
        while (position < text.size())
        {
            if (text[position] != '_' || position + 1 >= text.size())
            {
                label += text[position++];
                continue;
            }
            ++position;
            if (text[position] == '_')
                label += text[position++];
            else if (item.content.mnemonic == 0)
            {
                std::size_t letter = position;
                item.content.mnemonicAt = label.size();
                item.content.mnemonic = foldMnemonic(decodeUtf8(text, letter));
            }
        }
        item.content.text = label;
        return item.content.mnemonic != 0;
    }

    // Та же свёртка, что у сравнения строк списков (ASCII, Latin-1, греческий, кириллица) / The same folding as the list string comparison (ASCII, Latin-1, Greek, Cyrillic)
    char32_t foldMnemonic(char32_t symbol)
    {
        std::string text;
        std::size_t position = 0;

        appendUtf8(text, symbol);
        std::string folded = data::foldCase(text);
        return decodeUtf8(folded, position);
    }

    // Родитель — строка меню / The parent is a menu bar
    bool isMenuBarItem(const Node& node) noexcept
    {
        return node.kind == ElementKind::MENU_ITEM && node.parent != nullptr && node.parent->kind == ElementKind::MENU_BAR;
    }

    // Буква сравнивается в свёрнутом регистре: Alt+Ф и Alt+ф открывают «_Файл» / The letter is compared in folded case: Alt+Ф and Alt+ф open "_File"
    Node* mnemonicItem(Node& container, char32_t symbol)
    {
        char32_t folded = foldMnemonic(symbol);

        for (const std::unique_ptr<Node>& child : container.children)
        {
            if (child->kind == ElementKind::MENU_ITEM && child->visible && child->enabled && child->content.mnemonic != 0
                && child->content.mnemonic == folded)
                return child.get();
        }
        return nullptr;
    }

    /*
    ** Обход в глубину без рекурсии; слои (меню, диалоги) и скрытые ветви пропускаются: строка меню
    ** внутри диалога или выключенной вкладки клавишам Alt не отвечает.
    **
    ** A depth-first walk without recursion; the layers (menus, dialogs) and hidden branches are skipped: a menu bar
    ** inside a dialog or a hidden tab does not answer the Alt keys.
    */
    Node* menuBarOf(Node& root)
    {
        std::vector<Node*> pending{&root};

        while (!pending.empty())
        {
            Node* node = pending.back();
            pending.pop_back();
            if (!node->visible || (node != &root && isLayer(*node)))
                continue;
            if (node->kind == ElementKind::MENU_BAR)
                return node;
            for (std::size_t index = node->children.size(); index > 0; --index)
                pending.push_back(node->children[index - 1].get());
        }
        return nullptr;
    }
}
