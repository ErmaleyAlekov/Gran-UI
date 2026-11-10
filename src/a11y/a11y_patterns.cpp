#include "gran/a11y/access_tree.hpp"

namespace gran::a11y
{
    /*
    ** Шаблоны по роли, как у элементов Windows: кнопка и пункт меню нажимаются (пункт с подменю ещё и раскрывается), флажок и переключатель
    ** переключаются, радиокнопка и вкладка выбираются, поле и ComboBox имеют текстовое значение,
    ** ползунок и полоса прогресса — число в диапазоне. Любой другой узел с onClick тоже нажимается.
    **
    ** The patterns by role, as for Windows controls: a button and a menu item are pressed (an item with a submenu also expands), a check box and a switch
    ** are toggled, a radio button and a tab are selected, an input and a ComboBox have a text value,
    ** a slider and a progress bar have a number in the range. Any other node with onClick is pressed too.
    */
    std::uint8_t patternsOf(const ui::Node& node) noexcept
    {
        switch (roleOf(node))
        {
            case Role::MENU_ITEM:
                if (ui::submenuOf(node) != nullptr)
                    return PATTERN_INVOKE | PATTERN_EXPAND;
                return PATTERN_INVOKE;
            case Role::BUTTON: return PATTERN_INVOKE;
            case Role::CHECK_BOX: return PATTERN_TOGGLE;
            case Role::RADIO_BUTTON: case Role::TAB_ITEM: return PATTERN_SELECT;
            case Role::EDIT: case Role::COMBO_BOX: return PATTERN_TEXT;
            case Role::SLIDER: case Role::PROGRESS_BAR: case Role::THUMB: return PATTERN_RANGE;
            default: break;
        }
        if (!node.content.onClick.empty())
            return PATTERN_INVOKE;
        return 0;
    }

    // Те же элементы, что обходит Tab / The same controls that Tab walks
    bool isFocusable(const ui::Node& node) noexcept
    {
        switch (roleOf(node))
        {
            case Role::BUTTON: case Role::EDIT: case Role::CHECK_BOX: case Role::SLIDER: case Role::MENU_ITEM:
            case Role::COMBO_BOX: case Role::RADIO_BUTTON: case Role::TAB_ITEM: case Role::THUMB:
                return true;
            default:
                return false;
        }
    }

    // ComboBox меняется выбором пункта, полоса прогресса — только кодом / A ComboBox changes by choosing an item, a progress bar — only by the code
    bool isReadOnly(const ui::Node& node) noexcept
    {
        Role role = roleOf(node);

        if (role == Role::EDIT)
            return node.content.readOnly;
        return role == Role::COMBO_BOX || role == Role::PROGRESS_BAR;
    }

    // Копия запроса (строка) может не поместиться в память: исключение не уходит в ОС / The request copy (a string) may not fit into memory: the exception does not leave into the OS
    bool postRequest(AccessTree& tree, const AccessRequest& request) noexcept
    {
        try
        {
            tree.requests.push_back(request);
        }
        catch (...)
        {
            return false;
        }
        return true;
    }
}
