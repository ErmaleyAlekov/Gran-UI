#include "gran/render/internal/scene_internal.hpp"

#include <cmath>

namespace gran::render::internal
{
    /*
    ** Место каретки для байта показа caret. Сначала точное совпадение: байт после мягкого переноса
    ** стоит в начале следующей строки, а не в конце предыдущей. Затем конец строки, раскрытый в тот же
    ** байт (перед переводом строки, пустая последняя строка); иначе — последнее место текста.
    **
    ** The caret place for the display byte caret. First an exact match: the byte after a soft wrap
    ** stands at the start of the next line, not at the end of the previous one. Then a line end resolved
    ** into the same byte (before a line break, an empty last line); otherwise — the last text place.
    */
    text::CaretStop caretStopAt(const text::TextLayout& layout, std::size_t caret, const std::string& shown) noexcept
    {
        std::size_t found = layout.carets.size();

        for (std::size_t index = 0; index < layout.carets.size(); ++index)
        {
            if (layout.carets[index].cluster == caret)
                return layout.carets[index];
            if (found == layout.carets.size() && caretStopOffset(layout, index, shown) == caret)
                found = index;
        }
        if (found < layout.carets.size())
            return layout.carets[found];
        if (layout.carets.empty())
            return text::CaretStop{};
        return layout.carets.back();
    }

    // Черта шириной в 1.5 логических пикселя цветом текста (в выровненном случае — на целых пикселях)
    // A bar 1.5 logical pixels wide in the text color (in the aligned case — on whole pixels)
    void paintCaret(const ui::Node& node, Vec2 top, float height, const PaintState& state, SceneContext& context)
    {
        float unit = 1.0f;
        GpuInstance instance = placeInstance(state, Rect{top.x, top.y, 1.5f, height}, context.scale, unit);

        if (instance.rect[2] < 1.0f)
            instance.rect[2] = 1.0f;
        instance.kind = KIND_RECT;
        instance.fill = colorPackPremultiplied(node.computed.color);
        context.scene.instances.push_back(instance);
    }

    // Пустое поле — каретка в начале первой строки (раскладка там — подсказка) / An empty field — the caret at the first line start (the layout there is the placeholder)
    void paintTextCaret(const TextPlace& place, const PaintState& state, SceneContext& context)
    {
        const ui::Node& node = place.node;
        float lineHeight = node.computed.fontSize * node.computed.lineHeight;
        text::CaretStop caret;

        if (!node.content.text.empty())
            caret = caretStopAt(place.layout, toDisplayOffset(node, node.content.caret), displayedText(node));
        paintCaret(node, Vec2{place.origin.x + caret.x, place.origin.y + static_cast<float>(caret.line) * lineHeight},
            lineHeight, state, context);
    }
}
