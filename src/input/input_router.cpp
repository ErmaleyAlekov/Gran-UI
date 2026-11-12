#include "gran/input/input_router.hpp"

namespace gran::input
{
    // Курсор стиля переходит в платформу приведением (cursorShape): значения обязаны совпадать
    // The style cursor passes into the platform by a cast (cursorShape): the values must match
    static_assert(static_cast<int>(style::Cursor::DEFAULT) == static_cast<int>(platform::CursorShape::ARROW)
        && static_cast<int>(style::Cursor::POINTER) == static_cast<int>(platform::CursorShape::HAND)
        && static_cast<int>(style::Cursor::TEXT) == static_cast<int>(platform::CursorShape::TEXT)
        && static_cast<int>(style::Cursor::MOVE) == static_cast<int>(platform::CursorShape::MOVE)
        && static_cast<int>(style::Cursor::NOT_ALLOWED) == static_cast<int>(platform::CursorShape::NOT_ALLOWED)
        && static_cast<int>(style::Cursor::GRAB) == static_cast<int>(platform::CursorShape::GRAB)
        && static_cast<int>(style::Cursor::EW_RESIZE) == static_cast<int>(platform::CursorShape::RESIZE_EW)
        && static_cast<int>(style::Cursor::NS_RESIZE) == static_cast<int>(platform::CursorShape::RESIZE_NS), "Cursor");

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

    /*
    ** Мышь, затем клавиатура; потеря фокуса окном отпускает нажатие, захват и перетаскивание узла.
    ** Отложенные уведомления (focus, blur, enter, leave) доставляются в конце; вызванный обработчик — повод для кадра.
    **
    ** The mouse, then the keyboard; the window losing focus releases the press, the capture and a node drag.
    ** The deferred notifications (focus, blur, enter, leave) are delivered at the end; a called handler is a reason for a frame.
    */
    bool InputRouter::handle(const platform::Event& event)
    {
        bool changed = true;

        _state.delivered = 0;
        trackMenuCues(_state, event, false);
        if (event.kind == platform::EventKind::FOCUS_LOST)
        {
            cancelDrag(_state);
            changePressed(_state, nullptr);
            _state.drag = nullptr;
            _state.selecting = nullptr;
            _state.capture = nullptr;
        }
        else
        {
            // Признак клавиатуры живёт только на время события: слой, открытый кодом позже, фокус не забирает
            // The keyboard flag lives only during the event: a layer opened by code later does not take the focus
            _state.keyboard = event.kind == platform::EventKind::KEY_DOWN;
            changed = handlePointer(_state, event) || handleKeyboard(_state, event);
            _state.keyboard = false;
        }
        flushEvents(_state);
        if (trackMenuCues(_state, event, true))
            changed = true;
        return changed || _state.delivered != 0;
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
        // Перетаскивание узла: «переместить» над согласной зоной, иначе «нельзя» / A node drag: "move" over an agreeing zone, otherwise "no"
        if (_state.nodeDrag.active && _state.nodeDrag.allowed)
            return platform::CursorShape::MOVE;
        if (_state.nodeDrag.active)
            return platform::CursorShape::NOT_ALLOWED;
        // Ручка разделителя (и при перетаскивании): двусторонняя стрелка поперёк ручки / The splitter handle (and while dragging): a double arrow across the handle
        const ui::Node* handle = _state.hover;
        if (_state.drag != nullptr && (_state.drag->kind == ui::ElementKind::SPLITTER_HANDLE
            || _state.drag->kind == ui::ElementKind::DOCK_HANDLE))
            handle = _state.drag;
        bool grip = handle != nullptr
            && (handle->kind == ui::ElementKind::SPLITTER_HANDLE || handle->kind == ui::ElementKind::DOCK_HANDLE);
        if (grip && ui::splitterHorizontal(*handle))
            return platform::CursorShape::RESIZE_EW;
        if (grip)
            return platform::CursorShape::RESIZE_NS;
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
