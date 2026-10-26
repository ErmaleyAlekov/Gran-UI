#include "gran/render/internal/scene_internal.hpp"

#include <cmath>

namespace gran::render
{
    /*
    ** Прокрутка многострочного поля к каретке: строка каретки целиком в видимой части,
    ** сдвиг — минимальный (как прокрутка к каретке в браузере). Верхний предел смещения
    ** ставит компоновка (arrangeTextArea).
    **
    ** Scrolling a multi-line input to the caret: the caret line is entirely in the visible part,
    ** the shift is minimal (as the scroll to the caret in a browser). The upper offset limit
    ** is set by layout (arrangeTextArea).
    */
    bool scrollToCaret(TextEngine& engine, ui::Node& node)
    {
        Rect box = internal::contentRect(node);
        float lineHeight = node.computed.fontSize * node.computed.lineHeight;
        float& offset = node.layout.scrollOffset.y;
        float before = offset;
        float top = 0.0f;

        if (node.kind != ui::ElementKind::TEXT_AREA)
            return false;
        if (!node.content.text.empty())
        {
            const text::TextLayout& layout = engine.layoutFor(node, box.width);
            text::CaretStop caret = internal::caretStopAt(layout, toDisplayOffset(node, node.content.caret),
                displayedText(node));
            top = static_cast<float>(caret.line) * lineHeight;
        }
        if (top < offset)
            offset = top;
        if (top + lineHeight > offset + box.height)
            offset = top + lineHeight - box.height;
        offset = std::fmax(0.0f, offset);
        return offset != before;
    }
}
