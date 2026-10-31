#ifndef GRAN_UI_NODE_STATE_HPP
#define GRAN_UI_NODE_STATE_HPP

#include "gran/ui/node.hpp"

#include <map>
#include <memory>
#include <string>

namespace gran::ui
{
    /*
    ** Состояние узла, которое меняет пользователь или код, а не разметка: текст поля с кареткой
    ** и историей правок, отметка, значение ползунка и индикатора, выбранный пункт ComboBox и вкладка
    ** Tabs, сдвиг прокрутки. Его переносят на новые узлы при перестройке поддерева и перезагрузке
    ** разметки, чтобы правка файла не стирала набранное.
    **
    ** The node state the user or the code changes, not the markup: the field text with the caret
    ** and the edit history, the check mark, the slider and indicator value, the selected ComboBox item and the Tabs tab,
    ** the scroll offset. It is carried over to the new nodes when a subtree is rebuilt and the markup is reloaded,
    ** so editing the file does not erase what was typed.
    */
    struct SavedState
    {
        ElementKind kind = ElementKind::BOX;
        std::string text;
        std::size_t caret = 0;
        std::size_t anchor = 0;
        std::shared_ptr<TextHistory> history;
        bool checked = false;
        float value = 0.0f;
        std::string option;
        Vec2 scroll;
    };

    /*
    ** Снимок поддерева. Ключ узла — "#id", а у узла без id — ключ родителя и номер среди детей
    ** ("#panel/2/0"): так вставка в другой ветви не сдвигает ключи. focus — ключ узла в фокусе (пусто — нет).
    **
    ** A subtree snapshot. The node key is "#id", and for a node without an id — the parent key and the index among the children
    ** ("#panel/2/0"): this way an insertion in another branch does not shift the keys. focus is the focused node key (empty — none).
    */
    struct TreeState
    {
        std::map<std::string, SavedState> nodes;
        std::string focus;
        bool hasFocus = false;
    };

    // ---- node_state.cpp ----

    // Снимок состояния поддерева; focused — узел в фокусе окна (может быть вне поддерева или nullptr)
    // The subtree state snapshot; focused is the window focused node (may be outside the subtree or nullptr)
    TreeState captureState(const Node& subtree, const Node* focused);

    // ---- node_state_restore.cpp ----

    /*
    ** Вернуть состояние узлам с тем же ключом и видом. skipRoot — корень поддерева оставляет новое
    ** значение (код только что записал в разметку text, value или checked). Возвращает узел для фокуса
    ** или nullptr.
    **
    ** Give the state back to the nodes with the same key and kind. skipRoot — the subtree root keeps the new
    ** value (the code has just written text, value or checked into the markup). Returns the node to focus
    ** or nullptr.
    */
    Node* restoreState(Node& subtree, const TreeState& state, bool skipRoot);
}

#endif
