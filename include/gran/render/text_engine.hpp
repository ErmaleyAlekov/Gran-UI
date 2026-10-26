#ifndef GRAN_RENDER_TEXT_ENGINE_HPP
#define GRAN_RENDER_TEXT_ENGINE_HPP

#include "gran/ui/node.hpp"
#include "gran/text/text_layout.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace gran::render
{
    // Знак маски пароля «•» (U+2022) в UTF-8 и его длина в байтах / The password mask sign "•" (U+2022) in UTF-8 and its length in bytes
    constexpr const char* PASSWORD_MASK = "\xE2\x80\xA2";
    constexpr std::size_t PASSWORD_MASK_BYTES = 3;

    /*
    ** Текст узлов для компоновки и отрисовки. Раскладки кэшируются по полному ключу
    ** (текст, стиль, ширина), а не по адресу узла: удалённый и новый узел по тому же адресу
    ** не перепутаются. Неиспользованные за кадр записи удаляются в endFrame.
    **
    ** The node text for layout and rendering. The layouts are cached by the full key
    ** (the text, the style, the width), not by the node address: a removed and a new node at the same address
    ** do not get mixed up. Entries not used during a frame are removed in endFrame.
    */
    class TextEngine
    {
    public:
        // ---- text_engine.cpp ----
        explicit TextEngine(text::FontLibrary& fonts);

        // Размер текста узла (для компоновки) / The node text size (for layout)
        Size measureText(const ui::Node& node, float maxWidth);

        // Раскладка текста узла при ширине maxWidth / The node text layout at the width maxWidth
        const text::TextLayout& layoutFor(const ui::Node& node, float maxWidth);

        // Конец кадра: удаление неиспользованных раскладок / The frame end: removing the unused layouts
        void endFrame();

    private:
        // Кэшированная раскладка и номер кадра последнего обращения / A cached layout and the frame number of the last access
        struct Entry
        {
            text::TextLayout layout;
            std::uint32_t frame;
        };

        text::FontLibrary& _fonts;
        std::unordered_map<std::string, Entry> _cache;
        std::uint32_t _frame = 1;
    };

    // ---- scene/text_place.cpp ----

    // Байт UTF-8 места каретки, ближайшего к точке local (координаты компоновки узла) / The UTF-8 byte of the caret place nearest to the point local (the node layout coordinates)
    std::size_t textOffsetAt(TextEngine& engine, const ui::Node& node, Vec2 local);

    // ---- scene/text_lines.cpp ----

    // Байт начала (end = false) или конца строки, на которой стоит байт offset / The byte of the start (end = false) or the end of the line holding the byte offset
    std::size_t textLineEdge(TextEngine& engine, const ui::Node& node, std::size_t offset, bool end);

    // Байт на lines строк выше (< 0) или ниже; goalX — желаемый x (NaN — x байта offset), на выходе — использованный
    // The byte lines lines above (< 0) or below; goalX is the wanted x (NaN — the x of the byte offset), on output — the one used
    std::size_t textLineMove(TextEngine& engine, const ui::Node& node, std::size_t offset, int lines, float& goalX);

    // Строк в видимой части поля (шаг PageUp/PageDown) / The lines in the visible part of the field (the PageUp/PageDown step)
    int textPageLines(const ui::Node& node) noexcept;

    // ---- scene/text_reveal.cpp ----

    // Прокрутка TextArea так, чтобы строка каретки была видна; true — смещение изменилось
    // Scrolling a TextArea so that the caret line is visible; true — the offset changed
    bool scrollToCaret(TextEngine& engine, ui::Node& node);

    // ---- text_style.cpp ----

    /*
    ** Отображаемый текст узла: текст, подпись или подсказка поля; у поля пароля каждый символ
    ** заменён точкой «•» (U+2022, 3 байта UTF-8). Раскладка строится по нему, поэтому места
    ** каретки — в байтах показа; переводы — toDisplayOffset и toContentOffset (text_mask.cpp).
    **
    ** The displayed node text: the text, the label or the input placeholder; in a password input every character
    ** is replaced with the dot "•" (U+2022, 3 UTF-8 bytes). The layout is built from it, so the caret places
    ** are in display bytes; the conversions are toDisplayOffset and toContentOffset (text_mask.cpp).
    */
    std::string displayedText(const ui::Node& node);

    // ---- text_mask.cpp ----

    // Байт текста узла → байт показа (у пароля: число символов × 3) / A node text byte → a display byte (for a password: the character count × 3)
    std::size_t toDisplayOffset(const ui::Node& node, std::size_t offset) noexcept;

    // Байт показа → байт текста узла (на границе символа UTF-8) / A display byte → a node text byte (on a UTF-8 character boundary)
    std::size_t toContentOffset(const ui::Node& node, std::size_t offset) noexcept;

    // Параметры текста из вычисленного стиля / The text parameters from the computed style
    text::TextStyle textStyleOf(const ui::Node& node);

    // Ключ кэша: все параметры, влияющие на раскладку / The cache key: all parameters affecting the layout
    std::string layoutKey(const std::string& content, const text::TextStyle& style, float maxWidth);

    // Участки Text из детей Span (пусто — текст без участков) / The Text runs from the Span children (empty — a text without runs)
    std::vector<text::TextRun> textRunsOf(const ui::Node& node);

    // Добавка к ключу кэша: границы и стили участков / An addition to the cache key: the run bounds and styles
    std::string runsKey(const std::vector<text::TextRun>& runs);
}

#endif
