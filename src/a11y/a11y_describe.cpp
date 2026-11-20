#include "gran/a11y/access_tree.hpp"
#include "gran/input/input_router.hpp"
#include "gran/ui/data_grid.hpp"
#include "gran/utils/utf8.hpp"

namespace gran::a11y
{
    namespace
    {
        /*
        ** Части таблицы и дерева, чья роль зависит от места: своя прокрутка таблицы и дерева — оформление (строки
        ** читаются прямо как дети таблицы), первая строка GRID_HEADER — заголовки, вторая (строка фильтров) — группа
        ** полей; GRID_CELL в строке данных — ячейка, в строке фильтров — группа. false — вид без особого случая.
        **
        ** The table and tree parts whose role depends on the place: the own scroll of a table and a tree is layout (the rows
        ** are read right as the table children), the first GRID_HEADER line is the headers, the second (the filter row) is a group
        ** of inputs; a GRID_CELL in a data row is a cell, in the filter row — a group. false — a kind without a special case.
        */
        bool partRole(const ui::Node& node, Role& role) noexcept
        {
            using K = ui::ElementKind;
            const ui::Node* parent = node.parent;
            bool own = parent != nullptr && (parent->kind == K::DATA_GRID || parent->kind == K::TREE_VIEW);

            role = Role::GROUP;
            if (node.kind == K::SCROLL)
                return own;
            if (node.kind == K::GRID_HEADER)
            {
                if (own && ui::gridHeader(const_cast<ui::Node&>(*parent)) == &node)
                    role = Role::HEADER;
                return true;
            }
            if (node.kind != K::GRID_CELL)
                return false;
            if (parent != nullptr && parent->kind == K::GRID_ROW)
                role = Role::CELL;
            return true;
        }
    }

    /*
    ** Встроенные виды — свои роли (таблица), остальные контейнеры — группа. Переключатель — флажок с другим видом
    ** (как role="switch" поверх checkbox), кнопка «ещё» панели — кнопка.
    **
    ** The built-in kinds have their roles (a table), the other containers are groups. A switch is a check box with another look
    ** (as role="switch" over checkbox), the toolbar "more" button is a button.
    */
    Role roleOf(const ui::Node& node) noexcept
    {
        using K = ui::ElementKind;
        static const struct { K kind; Role role; } ROLES[] = {
            {K::TEXT, Role::TEXT}, {K::IMAGE, Role::IMAGE}, {K::ICON, Role::IMAGE}, {K::BUTTON, Role::BUTTON},
            {K::INPUT, Role::EDIT}, {K::TEXT_AREA, Role::EDIT}, {K::CHECKBOX, Role::CHECK_BOX}, {K::SWITCH, Role::CHECK_BOX},
            {K::RADIO_BUTTON, Role::RADIO_BUTTON}, {K::TABS, Role::TAB}, {K::TAB_HEADER, Role::TAB_ITEM},
            {K::PROGRESS_BAR, Role::PROGRESS_BAR}, {K::SLIDER, Role::SLIDER}, {K::SCROLL, Role::SCROLL}, {K::POPUP, Role::MENU},
            {K::MENU_ITEM, Role::MENU_ITEM}, {K::DIALOG, Role::DIALOG}, {K::COMBO_BOX, Role::COMBO_BOX},
            {K::MENU_BAR, Role::MENU_BAR}, {K::TOOL_BAR, Role::TOOL_BAR}, {K::SEPARATOR, Role::SEPARATOR},
            {K::TOOL_MORE, Role::BUTTON}, {K::STATUS_BAR, Role::STATUS_BAR}, {K::STATUS_SECTION, Role::TEXT},
            {K::SPLITTER_HANDLE, Role::THUMB}, {K::DOCK_PANEL, Role::PANE}, {K::DOCK_TITLE, Role::BUTTON},
            {K::DATA_GRID, Role::DATA_GRID}, {K::GRID_HEADER_CELL, Role::HEADER_ITEM}, {K::GRID_ROW, Role::DATA_ITEM},
            {K::TREE_VIEW, Role::TREE}, {K::TREE_ROW, Role::TREE_ITEM}};
        Role part = Role::GROUP;

        if (ui::isWindowButton(node))
            return Role::BUTTON;
        if (partRole(node, part))
            return part;
        for (const auto& entry : ROLES)
        {
            if (entry.kind == node.kind)
                return entry.role;
        }
        return Role::GROUP;
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
        info.checked = isSelected(node);
        info.expand = expandStateOf(node);
        info.focusable = isFocusable(node);
        info.readOnly = isReadOnly(node);
        // Кнопка раскрытия строки — тоже оформление: раскрывает сама строка (шаблон ExpandCollapse); пустой текст и стрелка — тоже
        // The row expander button is layout too: the row itself expands (the ExpandCollapse pattern); an empty text and an arrow too
        info.decorative = hiddenText(node)
            || (info.role == Role::GROUP && (content.onClick.empty() || node.kind == ui::ElementKind::TREE_EXPANDER));
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
