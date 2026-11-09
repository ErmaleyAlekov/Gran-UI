#include "gran/a11y/access_tree.hpp"
#include "gran/input/input_router.hpp"
#include "gran/utils/utf8.hpp"

namespace gran::a11y
{
    // Встроенные виды — свои роли, остальные контейнеры — группа / The built-in kinds have their roles, the other containers are groups
    Role roleOf(const ui::Node& node) noexcept
    {
        if (ui::isWindowButton(node))
            return Role::BUTTON;
        switch (node.kind)
        {
            case ui::ElementKind::TEXT: return Role::TEXT;
            case ui::ElementKind::IMAGE: case ui::ElementKind::ICON: return Role::IMAGE;
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
            case ui::ElementKind::MENU_BAR: return Role::MENU_BAR;
            default: return Role::GROUP;
        }
    }

    /*
    ** Клавиша доступа — буква подписи так, как она написана («Ф» из «_Файл»); у пункта строки меню с Alt,
    ** как её называет Windows («Alt+Ф»).
    **
    ** The access key is the label letter as written ("F" from "_File"); for a menu bar item with Alt,
    ** as Windows names it ("Alt+F").
    */
    std::string accessKeyOf(const ui::Node& node)
    {
        const std::string& label = node.content.text;
        std::size_t next = node.content.mnemonicAt;

        if (node.kind != ui::ElementKind::MENU_ITEM || node.content.mnemonic == 0 || next >= label.size())
            return std::string();
        decodeUtf8(label, next);
        std::string letter = label.substr(node.content.mnemonicAt, next - node.content.mnemonicAt);
        if (ui::isMenuBarItem(node))
            return "Alt+" + letter;
        return letter;
    }

    /*
    ** Значение: текст поля (кроме пароля), подпись выбранного пункта ComboBox или число ползунка и полосы
    ** прогресса. Узел внутри отключённого предка тоже отключён (как для мыши и клавиатуры).
    **
    ** The value: the input text (except a password), the selected ComboBox item label or the slider and progress bar
    ** number. A node inside a disabled ancestor is disabled too (as for the mouse and the keyboard).
    */
    AccessibleInfo describe(const ui::Node& node)
    {
        AccessibleInfo info;
        const ui::NodeContent& content = node.content;

        info.role = roleOf(node);
        info.name = accessibleName(node);
        info.help = content.tooltip;
        info.accessKey = accessKeyOf(node);
        info.enabled = !input::disabled(&node);
        info.checked = content.checked;
        info.focusable = isFocusable(node);
        info.readOnly = isReadOnly(node);
        info.decorative = info.role == Role::GROUP && content.onClick.empty();
        if ((info.role == Role::EDIT && !content.password) || info.role == Role::COMBO_BOX)
            info.value = content.text;
        info.minimum = content.minimum;
        info.maximum = content.maximum;
        info.current = content.value;
        // Шаг как у стрелок ползунка: step или сотая доля диапазона / The step as for the slider arrows: step or a hundredth of the range
        info.smallStep = content.step;
        if (info.smallStep <= 0.0f)
            info.smallStep = (content.maximum - content.minimum) / 100.0f;
        info.largeStep = info.smallStep * 10.0f;
        return info;
    }

    // Рамка через полное преобразование узла (поворот даёт описанный прямоугольник) / The frame through the full node transform (a rotation gives the bounding rectangle)
    Rect boundsOf(const ui::Node& node) noexcept
    {
        return affineBounds(ui::nodeToWindow(node), node.layout.frame);
    }
}
