#include "gran/layout/internal/layout_internal.hpp"

#include <cmath>

namespace gran::layout::internal
{
    namespace
    {
        // Ширина по умолчанию — 20 средних символов, как у Input и <textarea cols=20>
        // The default width is 20 average characters, as for Input and <textarea cols=20>
        float defaultWidth(const ui::Node& node) noexcept
        {
            return node.computed.fontSize * 0.55f * 20.0f;
        }

        // Высота строки поля / The field line height
        float lineHeightOf(const ui::Node& node) noexcept
        {
            return node.computed.fontSize * node.computed.lineHeight;
        }
    }

    /*
    ** Многострочное поле занимает доступную ширину (как блок), а без предела — ширину по умолчанию.
    ** Высота — по тексту при этой ширине, но не меньше rows строк; maxHeight ограничивает её
    ** в measureNode, а лишнее уходит под прокрутку (arrangeTextArea).
    **
    ** A multi-line input takes the available width (as a block), and without a limit — the default width.
    ** The height follows the text at that width, but not less than rows lines; maxHeight limits it
    ** in measureNode, and the excess goes under scrolling (arrangeTextArea).
    */
    Size measureTextArea(ui::Node& node, Size contentAvailable, LayoutPass& pass)
    {
        float width = defaultWidth(node);
        float rows = static_cast<float>(node.content.rows);

        if (std::isfinite(contentAvailable.width))
            width = contentAvailable.width;
        Size text = pass.measurer.measureText(node, width);
        return Size{width, std::fmax(text.height, rows * lineHeightOf(node))};
    }

    /*
    ** Размер содержимого — текст при ширине поля плюс padding (полосы прокрутки считают от края
    ** рамки, как у Scroll); смещение ограничивается так, чтобы под текстом не было пустоты.
    **
    ** The content size is the text at the field width plus padding (the scrollbars count from the border
    ** edge, as for Scroll); the offset is clamped so that no emptiness remains under the text.
    */
    void arrangeTextArea(ui::Node& node, Rect content, LayoutPass& pass)
    {
        const Insets& padding = node.computed.padding;
        float textHeight = std::fmax(pass.measurer.measureText(node, content.width).height, content.height);
        Vec2& offset = node.layout.scrollOffset;

        node.layout.contentSize = Size{content.width + padding.left + padding.right,
            textHeight + padding.top + padding.bottom};
        offset.x = 0.0f;
        offset.y = clampSize(offset.y, 0.0f, textHeight - content.height);
    }
}
