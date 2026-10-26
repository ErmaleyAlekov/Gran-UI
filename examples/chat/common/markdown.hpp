#ifndef CHAT_COMMON_MARKDOWN_HPP
#define CHAT_COMMON_MARKDOWN_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace chat
{
    // Оформление участка строки (биты) / The inline run styling (bits)
    enum InlineStyle : std::uint8_t
    {
        INLINE_PLAIN = 0,
        INLINE_BOLD = 1,
        INLINE_ITALIC = 2,
        INLINE_CODE = 4
    };

    // Участок строки: текст без разметки и его оформление / An inline run: the text without markup and its styling
    struct InlineRun
    {
        std::string text;
        std::uint8_t style = INLINE_PLAIN;
    };

    // Вид блока / The block kind
    enum class BlockKind : std::uint8_t { PARAGRAPH, HEADING, CODE };

    /*
    ** Блок ответа: абзац (строки внутри — через \n, маркеры списков заменены на «•»), заголовок
    ** уровня level или блок кода (весь текст — в одном участке, language — слово после ```).
    **
    ** A reply block: a paragraph (lines inside — through \n, list markers replaced with "•"), a heading
    ** of level level or a code block (the whole text is in one run, language is the word after ```).
    */
    struct MarkdownBlock
    {
        BlockKind kind = BlockKind::PARAGRAPH;
        int level = 0;
        std::string language;
        std::vector<InlineRun> runs;
    };

    // ---- markdown_blocks.cpp ----

    /*
    ** Простой Markdown ответов модели: блоки ``` (незакрытый — до конца текста: ответ ещё
    ** приходит), заголовки #..######, абзацы через пустую строку, списки «- », «* », «+ ».
    **
    ** Simple Markdown of model replies: ``` blocks (an unclosed one lasts to the text end: the reply is still
    ** coming), headings #..######, paragraphs separated by an empty line, lists "- ", "* ", "+ ".
    */
    std::vector<MarkdownBlock> parseMarkdown(std::string_view text);

    // ---- markdown_inline.cpp ----

    /*
    ** Участки строки: `код`, **жирный**, *курсив*. Незакрытый маркер остаётся текстом;
    ** соседние участки одного оформления сливаются.
    **
    ** The inline runs: `code`, **bold**, *italic*. An unclosed marker stays text;
    ** adjacent runs of the same styling merge.
    */
    std::vector<InlineRun> parseInline(std::string_view text);
}

#endif
