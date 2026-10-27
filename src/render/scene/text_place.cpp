#include "gran/render/internal/scene_internal.hpp"

#include <cmath>

namespace gran::render::internal
{
    /*
    ** Байт места каретки. Конец строки (страж 0xFFFFFFFF) — начало следующей строки или конец
    ** текста; если строка кончается переводом строки, место встаёт перед ним (как в браузере).
    **
    ** The byte of a caret place. A line end (the sentinel 0xFFFFFFFF) is the next line start or the text
    ** end; if the line ends with a line break, the place stands before it (as in a browser).
    */
    std::size_t caretStopOffset(const text::TextLayout& layout, std::size_t index, const std::string& text) noexcept
    {
        std::size_t offset = text.size();

        if (layout.carets[index].cluster != 0xFFFFFFFFu)
            return layout.carets[index].cluster;
        if (index + 1 < layout.carets.size())
            offset = layout.carets[index + 1].cluster;
        if (offset > text.size())
            offset = text.size();
        if (offset > 0 && text[offset - 1] == '\n')
            --offset;
        return offset;
    }

    /*
    ** Левый верхний угол раскладки в координатах компоновки: у кнопки и поля ввода — по центру
    ** по вертикали; поле сдвигается влево, если каретка ушла за правый край (сдвиг не зависит
    ** от мигания каретки, иначе текст прыгал бы). Многострочное поле сдвигается вверх на свою прокрутку.
    **
    ** The layout top-left corner in layout coordinates: for a button and an input — centered
    ** vertically; an input shifts left if the caret went beyond the right edge (the shift does not depend
    ** on the caret blinking, otherwise the text would jump). A multi-line input shifts up by its scroll.
    */
    Vec2 textOrigin(const ui::Node& node, const text::TextLayout& layout, const Rect& box)
    {
        Vec2 origin{box.x, box.y};
        float lineHeight = node.computed.fontSize * node.computed.lineHeight;

        if (node.kind == ui::ElementKind::BUTTON || node.kind == ui::ElementKind::INPUT
            || node.kind == ui::ElementKind::MENU_ITEM || node.kind == ui::ElementKind::COMBO_BOX)
            origin.y += (box.height - std::fmax(layout.height, lineHeight)) * 0.5f;
        if (node.kind == ui::ElementKind::TEXT_AREA)
            origin.y -= node.layout.scrollOffset.y;
        if (node.kind != ui::ElementKind::INPUT || node.content.text.empty())
            return origin;
        float caretX = caretStopAt(layout, toDisplayOffset(node, node.content.caret), displayedText(node)).x;
        if (caretX > box.width - 2.0f)
            origin.x -= caretX - box.width + 2.0f;
        return origin;
    }
}

namespace gran::render
{
    // Строка — по y (с ограничением первой и последней), место — ближайшее по x / The line by y (clamped to the first and the last), the place — the nearest by x
    std::size_t textOffsetAt(TextEngine& engine, const ui::Node& node, Vec2 local)
    {
        Rect box = internal::contentRect(node);
        const text::TextLayout& layout = engine.layoutFor(node, box.width);
        float lineHeight = node.computed.fontSize * node.computed.lineHeight;

        if (node.content.text.empty() || layout.lines.empty() || lineHeight <= 0.0f)
            return 0;
        Vec2 origin = internal::textOrigin(node, layout, box);
        float row = std::floor((local.y - origin.y) / lineHeight);
        float last = static_cast<float>(layout.lines.size() - 1);
        std::uint32_t line = static_cast<std::uint32_t>(std::fmin(std::fmax(row, 0.0f), last));
        std::size_t stop = internal::nearestStop(layout, line, local.x - origin.x, displayedText(node));
        if (stop == layout.carets.size())
            return 0;
        return toContentOffset(node, internal::caretStopOffset(layout, stop, displayedText(node)));
    }
}
