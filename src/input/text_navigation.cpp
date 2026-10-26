#include "gran/input/internal/text_cursor.hpp"

#include <limits>

namespace gran::input
{
    namespace
    {
        /*
        ** Новое место каретки по клавише. Стрелка без Shift при выделении схлопывает его
        ** к ближнему краю (как в браузере), иначе каретка шагает на символ UTF-8.
        **
        ** The new caret place by a key. An arrow without Shift with a selection collapses it
        ** to the near edge (as in a browser), otherwise the caret steps by a UTF-8 character.
        */
        bool arrowTarget(const ui::NodeContent& content, platform::Key key, bool shift, std::size_t& target)
        {
            std::size_t from = 0;
            std::size_t to = 0;
            bool collapse = internal::selectionRange(content, from, to) && !shift;

            if (key == platform::Key::LEFT && collapse)
                target = from;
            else if (key == platform::Key::LEFT)
                target = internal::previousBoundary(content.text, content.caret);
            else if (key == platform::Key::RIGHT && collapse)
                target = to;
            else if (key == platform::Key::RIGHT)
                target = internal::nextBoundary(content.text, content.caret);
            else if (key == platform::Key::HOME)
                target = 0;
            else if (key == platform::Key::END)
                target = content.text.size();
            else
                return false;
            return true;
        }

        // Home/End многострочного поля: края строки, с Ctrl — края всего текста / Home/End of a multi-line input: the line edges, with Ctrl — the whole text edges
        bool edgeTarget(RouterState& state, ui::Node& node, const platform::Event& event, std::size_t& target)
        {
            bool end = event.key == platform::Key::END;

            if (event.key != platform::Key::HOME && !end)
                return false;
            if ((event.modifiers & platform::MODIFIER_CONTROL) == 0)
                target = state.text->lineEdge(node, node.content.caret, end);
            else if (end)
                target = node.content.text.size();
            else
                target = 0;
            return true;
        }

        /*
        ** Многострочное поле: стрелки вверх и вниз — на строку, PageUp/PageDown — на видимую часть,
        ** Home/End — края строки. Желаемый x сохраняется между шагами подряд: каретка не сползает
        ** влево, проходя короткие строки (как в текстовых редакторах).
        **
        ** A multi-line input: the up and down arrows — by a line, PageUp/PageDown — by the visible part,
        ** Home/End — the line edges. The wanted x is kept between steps in a row: the caret does not drift
        ** left while passing short lines (as in text editors).
        */
        bool lineTarget(RouterState& state, ui::Node& node, const platform::Event& event, std::size_t& target)
        {
            int lines = 0;

            if (node.kind != ui::ElementKind::TEXT_AREA || state.text == nullptr)
                return false;
            if (edgeTarget(state, node, event, target))
                return true;
            if (event.key == platform::Key::UP)
                lines = -1;
            else if (event.key == platform::Key::DOWN)
                lines = 1;
            else if (event.key == platform::Key::PAGE_UP)
                lines = -state.text->pageLines(node);
            else if (event.key == platform::Key::PAGE_DOWN)
                lines = state.text->pageLines(node);
            else
                return false;
            if (state.goalNode != &node || state.goalCaret != node.content.caret)
                state.goalX = std::numeric_limits<float>::quiet_NaN();
            target = state.text->lineMove(node, node.content.caret, lines, state.goalX);
            state.goalNode = &node;
            state.goalCaret = target;
            return true;
        }
    }

    // Ctrl+A и Ctrl+C — общие для Text и полей; остальное — движение каретки (у TextArea — и по строкам)
    // Ctrl+A and Ctrl+C are common for a Text and the fields; the rest moves the caret (in a TextArea — by lines too)
    bool navigateText(RouterState& state, ui::Node& node, const platform::Event& event)
    {
        bool shift = (event.modifiers & platform::MODIFIER_SHIFT) != 0;
        bool control = (event.modifiers & platform::MODIFIER_CONTROL) != 0;
        std::size_t target = 0;

        if (control && event.key == platform::Key::A)
        {
            selectAll(state, node);
            return true;
        }
        // Пустое выделение ничего не копирует, но клавиша всё равно считается обработанной
        // An empty selection copies nothing, but the key still counts as handled
        if (control && event.key == platform::Key::C)
        {
            internal::copySelection(state, node);
            return true;
        }
        if (!lineTarget(state, node, event, target) && !arrowTarget(node.content, event.key, shift, target))
            return false;
        internal::placeCaret(node, target, shift);
        return true;
    }
}
