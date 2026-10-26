#include "common/markdown_internal.hpp"
#include "common/text_utils.hpp"

namespace chat
{
    namespace
    {
        // Блок кода в список: весь текст — один участок с оформлением кода / The code block into the list: the whole text is one run styled as code
        void pushCode(markdown::BlockParser& parser)
        {
            MarkdownBlock block;

            block.kind = BlockKind::CODE;
            block.language = parser.language;
            block.runs.push_back(InlineRun{parser.code, INLINE_CODE});
            parser.blocks.push_back(block);
            parser.inCode = false;
            parser.code.clear();
            parser.codeLines = 0;
        }

        /*
        ** Строка ``` открывает блок кода (слово после неё — язык) или закрывает открытый;
        ** внутри блока строки берутся как есть, без разбора разметки.
        **
        ** A ``` line opens a code block (the word after it is the language) or closes the open one;
        ** inside the block the lines are taken as is, without markup parsing.
        */
        void feedLine(markdown::BlockParser& parser, std::string_view line)
        {
            bool fence = markdown::stripIndent(line).compare(0, 3, "```") == 0;

            if (parser.inCode && fence)
                pushCode(parser);
            else if (parser.inCode)
            {
                if (parser.codeLines++ > 0)
                    parser.code += '\n';
                parser.code += line;
            }
            else if (fence)
            {
                markdown::flushParagraph(parser);
                parser.inCode = true;
                parser.language = std::string(trimSpaces(markdown::stripIndent(line).substr(3)));
            }
            else
                markdown::textLine(parser, line);
        }
    }

    // Текст режется на строки (\r\n и \n); незакрытый блок кода и последний абзац дописываются в конце / The text is cut into lines (\r\n and \n); an unclosed code block and the last paragraph are added at the end
    std::vector<MarkdownBlock> parseMarkdown(std::string_view text)
    {
        markdown::BlockParser parser;
        std::size_t start = 0;

        while (start <= text.size())
        {
            std::size_t end = text.find('\n', start);
            if (end == std::string_view::npos)
                end = text.size();
            std::string_view line = text.substr(start, end - start);
            if (!line.empty() && line.back() == '\r')
                line.remove_suffix(1);
            feedLine(parser, line);
            start = end + 1;
        }
        if (parser.inCode)
            pushCode(parser);
        markdown::flushParagraph(parser);
        return parser.blocks;
    }
}
