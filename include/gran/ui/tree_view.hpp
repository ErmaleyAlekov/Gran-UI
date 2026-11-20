#ifndef GRAN_UI_TREE_VIEW_HPP
#define GRAN_UI_TREE_VIEW_HPP

#include "gran/ui/node.hpp"

#include <memory>
#include <string>

namespace gran::ui
{
    // Отступ уровня дерева по умолчанию (логические пиксели) / The default tree level indent (logical pixels)
    constexpr float TREE_INDENT = 16.0f;

    /*
    ** Настройки дерева из разметки: field — поле записи для подписи строки (когда нет своего <ItemTemplate>),
    ** indent — сдвиг вправо на каждый уровень вложенности.
    **
    ** The tree settings from the markup: field is the record field for the row label (when there is no own <ItemTemplate>),
    ** indent is the shift to the right for every nesting level.
    */
    struct TreeViewOptions
    {
        std::string field;
        float indent = TREE_INDENT;
    };

    // ---- tree_view_build.cpp ----

    /*
    ** Части дерева TreeView над плоским деревом (data::TreeList): прокрутка и список строк. items, selection и virtual
    ** дерева переходят к списку; шаблон строки TREE_ROW — кнопка раскрытия со сдвигом по глубине и содержимое строки:
    ** копия <ItemTemplate> дерева (view.itemTemplate) или текст «{field}». Строки виртуальные, если virtual не задан явно.
    **
    ** The TreeView tree parts over a flat tree (data::TreeList): the scroll and the row list. The tree items, selection and virtual
    ** go to the list; the TREE_ROW row template is the expander button shifted by the depth and the row content:
    ** a copy of the tree <ItemTemplate> (view.itemTemplate) or the "{field}" text. The rows are virtual unless virtual is set explicitly.
    */
    void buildTreeView(Node& view, const TreeViewOptions& options);

    /*
    ** Кнопка раскрытия строки плоского дерева: сдвиг слева — глубина × indent (выражение строки $depth). Признаки
    ** ветки (content.branch — у узла есть дети, иначе стрелка не рисуется) и раскрытия (checked) ставит сама привязка
    ** списка, когда создаёт или обновляет копию строки (rowExpander), — без выражений и сравнения текста.
    ** Щелчок — встроенное действие gran:toggle. owner — узел разметки, чьё место в файле получает кнопка (для сообщений об ошибках).
    **
    ** The expander button of a flat tree row: the left shift is depth × indent (the $depth row expression). The branch flag
    ** (content.branch — the node has children, otherwise no arrow is painted) and the expanded one (checked) are set by
    ** the list binding itself when it creates or updates the row copy (rowExpander) — without expressions and text comparison.
    ** A click is the gran:toggle built-in action. owner is the markup node whose file place the button gets (for error messages).
    */
    std::unique_ptr<Node> makeTreeExpander(const Node& owner, float indent);

    // ---- tree_view_query.cpp ----

    // Список строк дерева (nullptr — не дерево или части не построены) / The tree row list (nullptr — not a tree or the parts are not built)
    Node* treeViewRows(Node& view) noexcept;

    // У кнопки раскрытия есть ветка: стрелка рисуется, щелчок раскрывает / The expander button has a branch: the arrow is painted, a click expands
    bool expanderHasBranch(const Node& expander) noexcept;

    // Кнопка раскрытия строки списка: ребёнок строки (TreeView) или первый ребёнок её ячейки (таблица-дерево); nullptr — нет
    // The list row expander button: a row child (TreeView) or the first child of its cell (a tree table); nullptr — none
    Node* rowExpander(Node& row) noexcept;
}

#endif
