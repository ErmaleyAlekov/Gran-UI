#include "gran/input/input_router.hpp"
#include "gran/input/internal/text_cursor.hpp"

namespace gran::input
{
    namespace
    {
        /*
        ** Узел доступен клиенту, как и мыши: он включён, а при открытом диалоге лежит в нём или в слое
        ** над ним (меню и список ComboBox диалога). Под модальным диалогом ничего не нажимается.
        **
        ** The node is reachable for a client as for the mouse: it is enabled, and with an open dialog it lies in it or in a layer
        ** over it (the dialog menus and ComboBox list). Nothing below a modal dialog is pressed.
        */
        bool reachable(const RouterState& state, const ui::Node& node) noexcept
        {
            const ui::Node* modal = modalLayer(state);
            bool above = false;

            if (disabled(&node))
                return false;
            if (modal == nullptr)
                return true;
            for (const OpenLayer& layer : state.layers)
            {
                above = above || layer.node == modal;
                if (above && within(&node, *layer.node))
                    return true;
            }
            return false;
        }

        /*
        ** Замена всего текста поля (шаблон Value): как «выделить всё и вставить» — те же очистка
        ** переводов строк, maxLength и onChange. Пустая строка стирает текст; отказ (только чтение, предел)
        ** возвращает прежнее выделение.
        **
        ** Replacing the whole input text (the Value pattern): as "select all and paste" — the same line break
        ** cleanup, maxLength and onChange. An empty string erases the text; a refusal (read-only, the limit)
        ** restores the former selection.
        */
        bool replaceText(RouterState& state, ui::Node& input, const std::string& text)
        {
            ui::NodeContent& content = input.content;
            std::size_t caret = content.caret;
            std::size_t anchor = content.anchor;

            if (!ui::isTextField(input) || content.readOnly || content.text == text)
                return false;
            content.anchor = 0;
            content.caret = content.text.size();
            if (text.empty())
                return internal::eraseSelection(state, input);
            if (insertText(state, input, text))
                return true;
            content.caret = caret;
            content.anchor = anchor;
            return false;
        }
    }

    /*
    ** Запрос выполняется теми же функциями, что и ввод человека: Invoke — как щелчок (activate),
    ** Toggle и Select — как выбор (changeChoice), значения — с теми же проверками и onChange.
    ** Провайдер хранит узлы константными, чтобы не менять их сам; дерево меняет только маршрутизатор.
    **
    ** The request is performed by the same functions as the human input: Invoke — as a click (activate),
    ** Toggle and Select — as a choice (changeChoice), the values — with the same checks and onChange.
    ** The provider keeps the nodes const so as not to change them itself; only the router changes the tree.
    */
    bool InputRouter::perform(const a11y::AccessRequest& request)
    {
        ui::Node& node = const_cast<ui::Node&>(*request.node);

        if (!reachable(_state, node))
            return false;
        switch (request.kind)
        {
            case a11y::Request::FOCUS: focus(&node); return true;
            case a11y::Request::INVOKE: activate(_state, node); return true;
            case a11y::Request::TOGGLE: changeChoice(_state, node); return true;
            case a11y::Request::SELECT: changeChoice(_state, node); return true;
            case a11y::Request::SET_TEXT: return replaceText(_state, node, request.text);
            case a11y::Request::SET_VALUE: break;
        }
        if (node.kind != ui::ElementKind::SLIDER)
            return false;
        return setSliderValue(_state, node, request.value);
    }
}
