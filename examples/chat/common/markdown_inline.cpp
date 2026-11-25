#include "common/markdown_internal.hpp"

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
        void toggle(InlineState& state, std::uint8_t bit)
        {
            pushRun(state.runs, state.current, state.style);
            state.current.clear();
            state.style = static_cast<std::uint8_t>(state.style ^ bit);
        }

        // `код` до закрывающей кавычки; разметка внутри не действует; false — закрывающей нет / `code` up to the closing backtick; markup inside does not act; false — no closing one
        bool codeSpan(InlineState& state, std::string_view rest, std::size_t& used)
        {
            used = markdown::codeSpanLength(rest);

            if (used == 1)
                return false;
            pushRun(state.runs, state.current, state.style);
            state.current.clear();
            pushRun(state.runs, std::string(rest.substr(1, used - 2)), static_cast<std::uint8_t>(state.style | INLINE_CODE));
            return true;
        }
    }

    // Проход слева направо: код, затем ** и *, иначе символ как есть / A left-to-right pass: code, then ** and *, otherwise the character as is
    std::vector<InlineRun> parseInline(std::string_view text)
    {
        auto markers = markdown::closingMarkers(text);
        InlineState state;
        std::size_t index = 0;

        while (index < text.size())
        {
            std::string_view rest = text.substr(index);
            std::size_t used = 1;
            if (rest[0] == '`' && codeSpan(state, rest, used))
                index += used;
            else if (rest[0] == '*')
            {
                bool bold = rest.size() > 1 && rest[1] == '*';
                auto bit = bold ? INLINE_BOLD : INLINE_ITALIC;
                used = bold ? 2 : 1;
                bool closing = (state.style & bit) != 0;
                bool valid = closing ? index > 0 && !markdown::markerSpace(text[index - 1])
                    : index < (bold ? markers.bold : markers.italic) && !markdown::markerSpace(rest[used]);
                if (valid)
                    toggle(state, bit);
                else
                {
                    // Без жирной пары вторая звёздочка может открыть курсив. / Without a bold pair, the second star may open italics.
                    if (bold && (state.style & INLINE_ITALIC) == 0 && !closing && index + 1 < markers.italic)
                        used = 1;
                    state.current.append(rest.substr(0, used));
                }
                index += used;
            }
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
