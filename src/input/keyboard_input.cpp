#include "gran/input/input_router.hpp"
#include "gran/utils/utf8.hpp"

namespace gran::input
{
    namespace
    {
        // Направление стрелки для ползунка (0 — не стрелка) / The arrow direction for a slider (0 — not an arrow)
        int arrowDirection(platform::Key key) noexcept
        {
            if (key == platform::Key::RIGHT || key == platform::Key::UP)
                return 1;
            if (key == platform::Key::LEFT || key == platform::Key::DOWN)
                return -1;
            return 0;
        }

        // Tab: следующий элемент; поле ввода, получившее фокус, выделяется целиком (как в браузере)
        // Tab: the next control; an input that got the focus is selected entirely (as in a browser)
        void focusNext(RouterState& state, bool backward)
        {
            changeFocus(state, nextFocusable(*state.root, state.focus, backward));
            if (state.focus != nullptr && state.focus->kind == ui::ElementKind::INPUT)
                selectAll(state, *state.focus);
        }

        /*
        ** Клавиша при фокусе: Tab — следующий элемент, Esc — снять фокус, поле ввода — правка,
        ** пробел и Enter — активация кнопки или флажка, стрелки — ползунок.
        ** Без фокуса клавиши идут в выделенный Text (Shift+стрелки, Ctrl+C, Ctrl+A).
        **
        ** A key with focus: Tab — the next control, Esc — remove the focus, an input — editing,
        ** space and Enter — activating a button or a check box, the arrows — a slider.
        ** Without focus the keys go to the selected Text (Shift+arrows, Ctrl+C, Ctrl+A).
        */
        bool keyDown(RouterState& state, const platform::Event& event)
        {
            ui::Node* focus = state.focus;

            if (popupKey(state, event))
                return true;
            if (event.key == platform::Key::TAB)
            {
                focusNext(state, (event.modifiers & platform::MODIFIER_SHIFT) != 0);
                return true;
            }
            if (focus == nullptr)
                return state.selection != nullptr && navigateText(state, *state.selection, event);
            if (event.key == platform::Key::ESCAPE)
            {
                changeFocus(state, nullptr);
                return true;
            }
            if (ui::isTextField(*focus))
                return editText(state, *focus, event);
            if (focus->kind == ui::ElementKind::SLIDER && arrowDirection(event.key) != 0)
                return stepSlider(state, *focus, arrowDirection(event.key));
            if (event.key != platform::Key::SPACE && event.key != platform::Key::ENTER)
                return false;
            activate(state, *focus);
            return true;
        }
    }

    // Нажатия и символы; символы идут только в поле ввода в фокусе / Presses and characters; characters go only into a focused input
    bool handleKeyboard(RouterState& state, const platform::Event& event)
    {
        std::string text;

        if (event.kind == platform::EventKind::KEY_DOWN)
            return keyDown(state, event);
        if (event.kind != platform::EventKind::CHARACTER || state.focus == nullptr)
            return false;
        if (!ui::isTextField(*state.focus) || state.focus->content.readOnly)
            return false;
        if ((event.modifiers & platform::MODIFIER_CONTROL) != 0 && (event.modifiers & platform::MODIFIER_ALT) == 0)
            return false;
        appendUtf8(text, event.character);
        return insertText(state, *state.focus, text);
    }
}
