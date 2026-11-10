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

        // Число в диапазоне от клиента: ползунок — значение, ручка разделителя — длина первой панели (и onChange)
        // A number in the range from a client: a slider — the value, a splitter handle — the first pane length (and onChange)
        bool setRangeValue(RouterState& state, ui::Node& node, float value)
        {
            if (node.kind == ui::ElementKind::SLIDER)
                return setSliderValue(state, node, value);
            if (node.kind != ui::ElementKind::SPLITTER_HANDLE || node.parent == nullptr || !setSplit(*node.parent, value))
                return false;
            finishSplit(state, *node.parent);
            return true;
        }

        /*
        ** Раскрыть подменю пункта (у пункта строки меню — закрыв её открытое меню: открыто одно) или свернуть его.
        ** false — подменю нет или оно уже в нужном состоянии.
        **
        ** Expanding the item submenu (for a menu bar item — closing its open menu: one is open) or collapsing it.
        ** false — there is no submenu or it is already in the needed state.
        */
        bool expandItem(RouterState& state, ui::Node& item, bool expand)
        {
            ui::Node* submenu = ui::submenuOf(item);

            if (submenu == nullptr || submenu->visible == expand)
                return false;
            if (!expand)
                closeLayer(state, *submenu);
            else if (ui::isMenuBarItem(item))
                closeMenus(state);
            if (expand)
                openSubmenu(state, item);
            return true;
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
        bool changed = true;

        if (!reachable(_state, node))
            return false;
        if (request.kind == a11y::Request::FOCUS)
            focus(&node);
        else if (request.kind == a11y::Request::INVOKE)
            activate(_state, node);
        else if (request.kind == a11y::Request::TOGGLE || request.kind == a11y::Request::SELECT)
            changeChoice(_state, node);
        else if (request.kind == a11y::Request::SET_TEXT)
            changed = replaceText(_state, node, request.text);
        else if (request.kind == a11y::Request::EXPAND || request.kind == a11y::Request::COLLAPSE)
            changed = expandItem(_state, node, request.kind == a11y::Request::EXPAND);
        else
            changed = setRangeValue(_state, node, request.value);
        // Фокус и выбор могли поставить уведомления focus/blur в очередь / The focus and the choice might queue focus/blur notifications
        flushEvents(_state);
        return changed;
    }
}
