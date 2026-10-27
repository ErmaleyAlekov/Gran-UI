#include "gran/input/input_router.hpp"

namespace gran::input
{
    // Встроенное контекстное меню добавляется в дерево один раз — при создании маршрутизатора
    // The built-in context menu is added to the tree once — when the router is created
    InputRouter::InputRouter(ui::Node& root, ActionSink& actions, platform::Window* window, TextLocator& text)
    {
        _state.root = &root;
        _state.actions = &actions;
        _state.window = window;
        _state.text = &text;
        _state.contextMenu = createContextMenu(root);
    }

    // Мышь, затем клавиатура; потеря фокуса окном отпускает нажатие / The mouse, then the keyboard; the window losing focus releases the press
    bool InputRouter::handle(const platform::Event& event)
    {
        if (event.kind == platform::EventKind::FOCUS_LOST)
        {
            changePressed(_state, nullptr);
            _state.drag = nullptr;
            _state.selecting = nullptr;
            return true;
        }
        // Признак клавиатуры живёт только на время события: слой, открытый кодом позже, фокус не забирает
        // The keyboard flag lives only during the event: a layer opened by code later does not take the focus
        _state.keyboard = event.kind == platform::EventKind::KEY_DOWN;
        bool changed = handlePointer(_state, event) || handleKeyboard(_state, event);
        _state.keyboard = false;
        return changed;
    }

    /*
    ** Курсор вычисленного стиля узла под мышью (cursor наследуется, как в CSS); над выделяемым
    ** Text со стилем по умолчанию — I-образный, как над текстом в браузере.
    **
    ** The computed style cursor of the hovered node (cursor is inherited, as in CSS); over a selectable
    ** Text with the default style — the I-beam, as over text in a browser.
    */
    platform::CursorShape InputRouter::cursor() const noexcept
    {
        if (_state.hover == nullptr)
            return platform::CursorShape::ARROW;
        if (_state.hover->computed.cursor == static_cast<int>(style::Cursor::DEFAULT)
            && textTarget(_state.hover, controlOf(_state.hover)) != nullptr)
            return platform::CursorShape::TEXT;
        if (disabled(_state.hover) && _state.hover->computed.cursor == static_cast<int>(style::Cursor::POINTER))
            return platform::CursorShape::NOT_ALLOWED;
        return static_cast<platform::CursorShape>(_state.hover->computed.cursor);
    }

    // Узел в фокусе / The focused node
    const ui::Node* InputRouter::focused() const noexcept
    {
        return _state.focus;
    }

    // Выделение поля ввода видно только в фокусе (как в браузере), выделение Text — всегда
    // An input selection is visible only with focus (as in a browser), a Text selection — always
    const ui::Node* InputRouter::selected() const noexcept
    {
        const ui::Node* node = _state.selection;

        if (node != nullptr && ui::isTextField(*node) && node != _state.focus)
            return nullptr;
        return node;
    }
}
