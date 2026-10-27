#include "gran/a11y/access_tree.hpp"

namespace gran::a11y
{
    namespace
    {
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
    }

    // Встроенные виды — свои роли, остальные контейнеры — группа / The built-in kinds have their roles, the other containers are groups
    Role roleOf(const ui::Node& node) noexcept
    {
        switch (node.kind)
        {
            case ui::ElementKind::TEXT: return Role::TEXT;
            case ui::ElementKind::IMAGE: return Role::IMAGE;
            case ui::ElementKind::BUTTON: return Role::BUTTON;
            case ui::ElementKind::INPUT: return Role::EDIT;
            case ui::ElementKind::TEXT_AREA: return Role::EDIT;
            // Переключатель — флажок с другим видом (как role="switch" поверх checkbox) / A switch is a check box with another look (as role="switch" over checkbox)
            case ui::ElementKind::CHECKBOX: case ui::ElementKind::SWITCH: return Role::CHECK_BOX;
            case ui::ElementKind::RADIO_BUTTON: return Role::RADIO_BUTTON;
            case ui::ElementKind::TABS: return Role::TAB;
            case ui::ElementKind::TAB_HEADER: return Role::TAB_ITEM;
            case ui::ElementKind::PROGRESS_BAR: return Role::PROGRESS_BAR;
            case ui::ElementKind::SLIDER: return Role::SLIDER;
            case ui::ElementKind::SCROLL: return Role::SCROLL;
            case ui::ElementKind::POPUP: return Role::MENU;
            case ui::ElementKind::MENU_ITEM: return Role::MENU_ITEM;
            case ui::ElementKind::DIALOG: return Role::DIALOG;
            case ui::ElementKind::COMBO_BOX: return Role::COMBO_BOX;
            default: return Role::GROUP;
        }
    }

    /*
    ** Имя как в HTML: у поля ввода и ComboBox — подсказка (placeholder; текст — это значение), у остальных —
    ** свой текст, у кнопки без текста — тексты детей; последний запасной вариант — tooltip.
    **
    ** The name as in HTML: for an input and a ComboBox — the placeholder (the text is the value), for the others —
    ** their own text, for a button without text — the children texts; the last fallback is the tooltip.
    */
    std::string accessibleName(const ui::Node& node)
    {
        std::string name;

        if (ui::isTextField(node) || node.kind == ui::ElementKind::COMBO_BOX)
            name = node.content.placeholder;
        else
            name = node.content.text;
        if (name.empty() && node.kind == ui::ElementKind::BUTTON)
            appendTexts(node, name);
        if (name.empty())
            name = node.content.tooltip;
        return name;
    }

    // Значение: текст поля (кроме пароля), подпись выбранного пункта ComboBox или число ползунка и полосы прогресса
    // The value: the input text (except a password), the selected ComboBox item label or the slider and progress bar number
    AccessibleInfo describe(const ui::Node& node)
    {
        AccessibleInfo info;

        info.role = roleOf(node);
        info.name = accessibleName(node);
        info.help = node.content.tooltip;
        info.enabled = node.enabled;
        info.checked = node.content.checked;
        info.focusable = info.role == Role::BUTTON || info.role == Role::EDIT || info.role == Role::CHECK_BOX
            || info.role == Role::SLIDER || info.role == Role::MENU_ITEM || info.role == Role::COMBO_BOX
            || info.role == Role::RADIO_BUTTON || info.role == Role::TAB_ITEM;
        info.decorative = info.role == Role::GROUP && node.content.onClick.empty();
        if ((info.role == Role::EDIT && !node.content.password) || info.role == Role::COMBO_BOX)
            info.value = node.content.text;
        info.minimum = node.content.minimum;
        info.maximum = node.content.maximum;
        info.current = node.content.value;
        return info;
    }

    // Рамка через полное преобразование узла (поворот даёт описанный прямоугольник) / The frame through the full node transform (a rotation gives the bounding rectangle)
    Rect boundsOf(const ui::Node& node) noexcept
    {
        return affineBounds(ui::nodeToWindow(node), node.layout.frame);
    }
}
