#include "gran/input/internal/text_cursor.hpp"

#include <limits>

namespace gran::input
{
    namespace
    {
        // Стрелка без Shift при выделении схлопывает его к ближнему краю (как в браузере)
        // An arrow without Shift with a selection collapses it to the near edge (as in a browser)
        bool collapseTarget(const ui::NodeContent& content, internal::TextMove move, bool shift, std::size_t& target)
        {
            std::size_t from = 0;
            std::size_t to = 0;

            if (shift || !internal::selectionRange(content, from, to))
                return false;
            if (move == internal::TextMove::CHAR_BACK)
                target = from;
            else if (move == internal::TextMove::CHAR_FORWARD)
                target = to;
            else
                return false;
            return true;
        }

        // Края строки (у однострочного поля — края текста) и края всего текста
        // The line edges (in a single-line field — the text edges) and the whole text edges
        bool edgeTarget(RouterState& state, ui::Node& node, internal::TextMove move, std::size_t& target)
        {
            bool line = move == internal::TextMove::LINE_START || move == internal::TextMove::LINE_END;
            bool end = move == internal::TextMove::LINE_END || move == internal::TextMove::TEXT_END;

            if (!line && !end && move != internal::TextMove::TEXT_START)
                return false;
            if (line && node.kind == ui::ElementKind::TEXT_AREA && state.text != nullptr)
                target = state.text->lineEdge(node, node.content.caret, end);
            else if (end)
                target = node.content.text.size();
            else
                target = 0;
            return true;
        }

        /*
        ** Многострочное поле: строка вверх и вниз, PageUp/PageDown — на видимую часть. Желаемый x сохраняется
        ** между шагами подряд: каретка не сползает влево, проходя короткие строки (как в текстовых редакторах).
        **
        ** A multi-line input: a line up and down, PageUp/PageDown — by the visible part. The wanted x is kept
        ** between steps in a row: the caret does not drift left while passing short lines (as in text editors).
        */
        bool lineTarget(RouterState& state, ui::Node& node, internal::TextMove move, std::size_t& target)
        {
            int lines = 0;

            if (node.kind != ui::ElementKind::TEXT_AREA || state.text == nullptr)
                return false;
            if (move == internal::TextMove::LINE_UP)
                lines = -1;
            else if (move == internal::TextMove::LINE_DOWN)
                lines = 1;
            else if (move == internal::TextMove::PAGE_UP)
                lines = -state.text->pageLines(node);
            else if (move == internal::TextMove::PAGE_DOWN)
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

    // Символ и слово считаются по тексту; края и строки — по раскладке поля / A character and a word are counted by the text; the edges and lines — by the field layout
    bool internal::moveTarget(RouterState& state, ui::Node& node, TextMove move, std::size_t& target)
    {
        const std::string& text = node.content.text;
        std::size_t caret = node.content.caret;

        switch (move)
        {
            case TextMove::CHAR_BACK: target = previousBoundary(text, caret); return true;
            case TextMove::CHAR_FORWARD: target = nextBoundary(text, caret); return true;
            case TextMove::WORD_BACK: target = wordStartBefore(text, caret); return true;
            case TextMove::WORD_END: target = wordEndAfter(text, caret); return true;
            case TextMove::WORD_NEXT: target = wordNextStart(text, caret); return true;
            case TextMove::PARAGRAPH_BACK: target = paragraphStart(text, caret, true); return true;
            case TextMove::PARAGRAPH_FORWARD: target = paragraphEnd(text, caret, true); return true;
            case TextMove::PARAGRAPH_START: target = paragraphStart(text, caret, false); return true;
            case TextMove::PARAGRAPH_END: target = paragraphEnd(text, caret, false); return true;
            default: break;
        }
        return edgeTarget(state, node, move, target) || lineTarget(state, node, move, target);
    }

    /*
    ** Ctrl+A и Ctrl+C (Cmd на macOS) — общие для Text и полей; остальное — шаг каретки по смыслу клавиши
    ** в соглашениях ОС сборки (internal::caretMove).
    **
    ** Ctrl+A and Ctrl+C (Cmd on macOS) are common for a Text and the fields; the rest is a caret step by the key
    ** meaning in the build OS conventions (internal::caretMove).
    */
    bool navigateText(RouterState& state, ui::Node& node, const platform::Event& event)
    {
        bool shift = (event.modifiers & platform::MODIFIER_SHIFT) != 0;
        bool control = (event.modifiers & platform::MODIFIER_CONTROL) != 0;
        internal::TextMove move = internal::caretMove(event.key, event.modifiers, platform::hostOs());
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
        if (move == internal::TextMove::NONE
            || (!collapseTarget(node.content, move, shift, target) && !internal::moveTarget(state, node, move, target)))
            return false;
        internal::placeCaret(node, target, shift);
        return true;
    }
}
