#include "gran/render/text_engine.hpp"
#include "gran/utils/utf8.hpp"

#include <cmath>
#include <cstdio>

namespace gran::render
{
    // Выравнивание стиля переходит в текст приведением (textStyle): значения обязаны совпадать
    // The style alignment passes into the text by a cast (textStyle): the values must match
    static_assert(static_cast<int>(style::TextAlign::START) == static_cast<int>(text::TextAlignment::START)
        && static_cast<int>(style::TextAlign::CENTER) == static_cast<int>(text::TextAlignment::CENTER)
        && static_cast<int>(style::TextAlign::END) == static_cast<int>(text::TextAlignment::END)
        && static_cast<int>(style::TextAlign::JUSTIFY) == static_cast<int>(text::TextAlignment::JUSTIFY), "TextAlign");

    // Поле ввода и ComboBox без значения показывают подсказку, поле пароля — точки, остальные — свой текст
    // An input and a ComboBox without a value show the placeholder, a password input — dots, the others show their text
    std::string displayedText(const ui::Node& node)
    {
        if ((ui::isTextField(node) || node.kind == ui::ElementKind::COMBO_BOX) && node.content.text.empty())
            return node.content.placeholder;
        if (!node.content.password)
            return node.content.text;
        std::size_t count = countUtf8(node.content.text);
        std::string masked;
        masked.reserve(count * PASSWORD_MASK_BYTES);
        for (std::size_t index = 0; index < count; ++index)
            masked += PASSWORD_MASK;
        return masked;
    }

    // Перевод вычисленного стиля в параметры текстового движка / Turning the computed style into the text engine parameters
    text::TextStyle textStyleOf(const ui::Node& node)
    {
        const style::ComputedStyle& computed = node.computed;
        text::TextStyle style;

        style.family = computed.fontFamily;
        style.size = computed.fontSize;
        style.weight = computed.fontWeight;
        style.italic = computed.fontStyle == 1;
        style.lineHeight = computed.lineHeight;
        style.letterSpacing = computed.letterSpacing;
        style.align = static_cast<text::TextAlignment>(computed.textAlign);
        style.wrap = computed.textWrap == static_cast<int>(style::TextWrap::WRAP);
        style.ellipsis = computed.textOverflow == static_cast<int>(style::TextOverflow::ELLIPSIS);
        style.maxLines = computed.maxLines;
        // Однострочное поле не переносит; многострочное переносит всегда и показывает весь текст (правка без многоточия)
        // A single-line input never wraps; a multi-line one always wraps and shows the whole text (editing without an ellipsis)
        if (node.kind == ui::ElementKind::INPUT)
            style.wrap = false;
        if (node.kind == ui::ElementKind::TEXT_AREA)
        {
            style.wrap = true;
            style.ellipsis = false;
            style.maxLines = 0;
        }
        return style;
    }

    /*
    ** Ключ из всех полей; ширина округляется до 1/64 px, бесконечность пишется словом:
    ** близкие ширины из гибкой компоновки дают одну запись.
    **
    ** The key from all fields; the width is rounded to 1/64 px, infinity is written as a word:
    ** close widths from flexible layout give one entry.
    */
    std::string layoutKey(const std::string& content, const text::TextStyle& style, float maxWidth)
    {
        char numbers[160];
        long width = -1;

        if (std::isfinite(maxWidth))
            width = std::lround(maxWidth * 64.0f);
        std::snprintf(numbers, sizeof(numbers), "|%g|%d|%d|%g|%g|%d|%d|%d|%d|%ld|",
            style.size, style.weight, static_cast<int>(style.italic), style.lineHeight, style.letterSpacing,
            static_cast<int>(style.align), static_cast<int>(style.wrap), static_cast<int>(style.ellipsis),
            style.maxLines, width);
        return style.family + numbers + content;
    }

    // Границы участков идут подряд по длинам текстов детей / The run bounds follow one another by the children text lengths
    std::vector<text::TextRun> textRunsOf(const ui::Node& node)
    {
        std::vector<text::TextRun> runs;
        std::uint32_t begin = 0;

        if (!ui::hasSpans(node))
            return runs;
        for (const std::unique_ptr<ui::Node>& span : node.children)
        {
            std::uint32_t end = begin + static_cast<std::uint32_t>(span->content.text.size());
            runs.push_back(text::TextRun{begin, end, textStyleOf(*span)});
            begin = end;
        }
        return runs;
    }

    // Ключ каждого участка — его начало и ключ его стиля без текста / The key of each run is its start and its style key without text
    std::string runsKey(const std::vector<text::TextRun>& runs)
    {
        std::string key;

        for (const text::TextRun& run : runs)
            key += "#" + std::to_string(run.begin) + layoutKey(std::string(), run.style, -1.0f);
        return key;
    }
}
