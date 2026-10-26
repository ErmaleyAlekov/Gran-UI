#include "common/markdown.hpp"

namespace chat
{
    namespace
    {
        // Состояние разбора: готовые участки, копящийся текст и текущее оформление / The parse state: the finished runs, the accumulating text and the current styling
        struct InlineState
        {
            std::vector<InlineRun> runs;
            std::string current;
            std::uint8_t style = INLINE_PLAIN;
        };

        // Участок в конец: пустой пропускается, участок того же оформления дописывается / A run to the end: an empty one is skipped, a run of the same styling is extended
        void pushRun(std::vector<InlineRun>& runs, const std::string& text, std::uint8_t style)
        {
            if (text.empty())
                return;
            if (!runs.empty() && runs.back().style == style)
            {
                runs.back().text += text;
                return;
            }
            runs.push_back(InlineRun{text, style});
        }

        /*
        ** Маркер ** или * переключает оформление. Открыть можно, только если дальше есть такой же
        ** закрывающий и сразу за маркером не пробел («* пункт», «2 * 3» — не курсив).
        **
        ** The ** or * marker toggles the styling. It may open only if a matching closing one follows
        ** and the marker is not followed by a space ("* item", "2 * 3" are not italic).
        */
        bool toggle(InlineState& state, std::string_view rest, std::string_view marker, std::uint8_t bit)
        {
            bool open = (state.style & bit) == 0;

            if (open && (rest.size() <= marker.size() || rest[marker.size()] == ' '))
                return false;
            if (open && rest.find(marker, marker.size()) == std::string_view::npos)
                return false;
            pushRun(state.runs, state.current, state.style);
            state.current.clear();
            state.style = static_cast<std::uint8_t>(state.style ^ bit);
            return true;
        }

        // `код` до закрывающей кавычки; разметка внутри не действует; false — закрывающей нет / `code` up to the closing backtick; markup inside does not act; false — no closing one
        bool codeSpan(InlineState& state, std::string_view rest, std::size_t& used)
        {
            std::size_t close = rest.find('`', 1);

            if (close == std::string_view::npos || close == 1)
                return false;
            pushRun(state.runs, state.current, state.style);
            state.current.clear();
            pushRun(state.runs, std::string(rest.substr(1, close - 1)), static_cast<std::uint8_t>(state.style | INLINE_CODE));
            used = close + 1;
            return true;
        }
    }

    // Проход слева направо: код, затем ** и *, иначе символ как есть / A left-to-right pass: code, then ** and *, otherwise the character as is
    std::vector<InlineRun> parseInline(std::string_view text)
    {
        InlineState state;
        std::size_t index = 0;

        while (index < text.size())
        {
            std::string_view rest = text.substr(index);
            std::size_t used = 1;
            bool doubled = rest.size() > 1 && rest[1] == '*';
            if (rest[0] == '`' && codeSpan(state, rest, used))
                index += used;
            else if (rest[0] == '*' && doubled && toggle(state, rest, "**", INLINE_BOLD))
                index += 2;
            else if (rest[0] == '*' && !doubled && toggle(state, rest, "*", INLINE_ITALIC))
                index += 1;
            else
            {
                state.current += rest[0];
                ++index;
            }
        }
        pushRun(state.runs, state.current, state.style);
        return state.runs;
    }
}
