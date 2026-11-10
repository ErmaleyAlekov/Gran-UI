#include "gran/input/input_router.hpp"
#include "gran/input/internal/text_history.hpp"
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

        /*
        ** Tab: следующий элемент — в открытом диалоге только среди его элементов (фокус не уходит
        ** под затемнение); поле ввода, получившее фокус, выделяется целиком (как в браузере).
        **
        ** Tab: the next control — in an open dialog only among its controls (the focus does not go
        ** under the dimming); an input that got the focus is selected entirely (as in a browser).
        */
        void focusNext(RouterState& state, bool backward)
        {
            ui::Node* scope = modalLayer(state);

            if (scope == nullptr)
                scope = state.root;
            changeFocus(state, nextFocusable(*scope, state.focus, backward));
            if (state.focus != nullptr && state.focus->kind == ui::ElementKind::INPUT)
                selectAll(state, *state.focus);
        }

        // Tab и Shift+Tab — обход, Esc при фокусе — снять его; true — обработано / Tab and Shift+Tab — the walk, Esc with focus — removing it; true — handled
        bool focusKey(RouterState& state, const platform::Event& event)
        {
            if (event.key == platform::Key::TAB)
            {
                focusNext(state, (event.modifiers & platform::MODIFIER_SHIFT) != 0);
                return true;
            }
            if (event.key != platform::Key::ESCAPE || state.focus == nullptr)
                return false;
            changeFocus(state, nullptr);
            return true;
        }

        /*
        ** Клавиша при фокусе: Tab — следующий элемент, Esc — снять фокус, поле ввода — правка,
        ** пробел и Enter — активация кнопки или флажка, стрелки — ползунок и выбор в группе (радиокнопки, вкладки), стрелка вниз — открыть
        ** меню кнопки или список ComboBox. Без фокуса клавиши идут в выделенный Text (Shift+стрелки, Ctrl+C, Ctrl+A).
        **
        ** A key with focus: Tab — the next control, Esc — remove the focus, an input — editing,
        ** space and Enter — activating a button or a check box, the arrows — a slider and the choice in a group (radio buttons, tabs), the down arrow — opening
        ** the button menu or the ComboBox list. Without focus the keys go to the selected Text (Shift+arrows, Ctrl+C, Ctrl+A).
        */
        bool keyDown(RouterState& state, const platform::Event& event)
        {
            ui::Node* focus = state.focus;

            if (popupKey(state, event) || menuBarKey(state, event) || focusKey(state, event))
                return true;
            if (focus == nullptr)
                return state.selection != nullptr && navigateText(state, *state.selection, event);
            if (ui::isTextField(*focus))
                return editText(state, *focus, event);
            if (!focus->content.selection.empty() && state.actions->listKey(*focus, event.key, event.modifiers))
                return true;
            if (stepChoice(state, *focus, event.key))
                return true;
            if (focus->kind == ui::ElementKind::SPLITTER_HANDLE && splitterKey(state, *focus, event))
                return true;
            if (focus->kind == ui::ElementKind::SLIDER && arrowDirection(event.key) != 0)
                return stepSlider(state, *focus, arrowDirection(event.key));
            if (event.key == platform::Key::DOWN)
                return openAttached(state, *focus);
            if (event.key != platform::Key::SPACE && event.key != platform::Key::ENTER)
                return false;
            activate(state, *focus);
            return true;
        }
    }

    /*
    ** Нажатия и символы. keyDown сначала идёт приложению: отменённый — ни встроенной обработки, ни символов
    ** этой клавиши в поле (как preventDefault у keydown в браузере); затем Alt и F10 (строка меню), горячие клавиши
    ** команд (выполненная команда гасит и символ своей клавиши); keyUp — приложению и отпусканию Alt. Символ сначала
    ** пробуется как буква доступа меню, затем идёт в поле ввода в фокусе.
    **
    ** Presses and characters. keyDown goes to the application first: a cancelled one gets neither the built-in handling nor the characters
    ** of this key in a field (as preventDefault on keydown in a browser); then Alt and F10 (the menu bar), the command shortcuts
    ** (a command that ran also drops the character of its key); keyUp goes to the application and to the Alt release. A character
    ** is first tried as a menu access letter, then it goes into the focused input.
    */
    bool handleKeyboard(RouterState& state, const platform::Event& event)
    {
        std::string text;

        if (event.kind == platform::EventKind::KEY_DOWN)
        {
            if (dragKey(state, event))
                return true;
            state.suppressCharacter = !fireKey(state, event);
            if (state.suppressCharacter || altKey(state, event) || contextKey(state, event))
                return true;
            state.suppressCharacter = commandKey(state, event);
            return state.suppressCharacter || keyDown(state, event);
        }
        if (event.kind == platform::EventKind::KEY_UP)
        {
            // Отпускание Alt учитывается и при отменённом keyUp: иначе Alt остался бы «зажатым»
            // The Alt release counts even with a cancelled keyUp: otherwise Alt would stay "held"
            fireKey(state, event);
            return altKey(state, event);
        }
        if (event.kind != platform::EventKind::CHARACTER || state.suppressCharacter)
            return false;
        if (mnemonicKey(state, event))
            return true;
        if (state.focus == nullptr || !ui::isTextField(*state.focus) || state.focus->content.readOnly
            || ((event.modifiers & platform::MODIFIER_CONTROL) != 0 && (event.modifiers & platform::MODIFIER_ALT) == 0))
            return false;
        appendUtf8(text, event.character);
        return internal::typeText(state, *state.focus, text);
    }
}
