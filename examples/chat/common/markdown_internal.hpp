#ifndef CHAT_COMMON_MARKDOWN_INTERNAL_HPP
#define CHAT_COMMON_MARKDOWN_INTERNAL_HPP

#include "common/markdown.hpp"

namespace chat::markdown
{
    /*
    ** Накопитель блоков: строки абзаца копятся в paragraph, строки кода — в code до закрывающей ```;
    ** codeLines — сколько строк кода уже принято (первая пустая строка кода тоже строка).
    **
    ** The block accumulator: the paragraph lines gather in paragraph, the code lines — in code until the closing ```;
    ** codeLines is how many code lines were taken already (an empty first code line is a line too).
    */
    struct BlockParser
    {
        std::vector<MarkdownBlock> blocks;
        std::string paragraph;
        std::string code;
        std::string language;
        std::size_t codeLines = 0;
        bool inCode = false;
    };

    // ---- markdown_lines.cpp ----

    // Отступ до трёх пробелов не меняет смысла строки (как в CommonMark) / An indent of up to three spaces does not change the line meaning (as in CommonMark)
    std::string_view stripIndent(std::string_view line) noexcept;

    // Накопленный абзац → блок с участками / The accumulated paragraph → a block with runs
    void flushParagraph(BlockParser& parser);

    // Строка вне кода: заголовок, пустая строка (конец абзаца) или строка абзаца / A line outside code: a heading, an empty line (the paragraph end) or a paragraph line
    void textLine(BlockParser& parser, std::string_view line);
}

#endif
