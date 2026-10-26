#include "common/markdown_internal.hpp"
#include "common/text_utils.hpp"

namespace chat::markdown
{
    namespace
    {
        // «# Заголовок»: от одного до шести # и пробел; true — строка стала блоком заголовка / "# Heading": one to six # and a space; true — the line became a heading block
        bool headingLine(BlockParser& parser, std::string_view body)
        {
            std::size_t level = 0;
            MarkdownBlock heading;

            while (level < body.size() && level < 6 && body[level] == '#')
                ++level;
            if (level == 0 || level >= body.size() || body[level] != ' ')
                return false;
            flushParagraph(parser);
            heading.kind = BlockKind::HEADING;
            heading.level = static_cast<int>(level);
            heading.runs = parseInline(trimSpaces(body.substr(level + 1)));
            parser.blocks.push_back(heading);
            return true;
        }

        // Маркер списка «- », «* », «+ » (после любого отступа) → «• »; отступ вложенного списка сохраняется / A list marker "- ", "* ", "+ " (after any indent) → "• "; a nested list indent is kept
        std::string listLine(std::string_view line)
        {
            std::size_t first = line.find_first_not_of(' ');

            if (first == std::string_view::npos || first + 1 >= line.size() || line[first + 1] != ' ')
                return std::string(line);
            if (line[first] != '-' && line[first] != '*' && line[first] != '+')
                return std::string(line);
            return std::string(line.substr(0, first)) + "\xE2\x80\xA2 " + std::string(line.substr(first + 2));
        }
    }

    // Пробелы после трёх первых — уже отступ блока кода в CommonMark; здесь они просто остаются / Spaces after the first three are a code block indent in CommonMark; here they simply stay
    std::string_view stripIndent(std::string_view line) noexcept
    {
        std::size_t spaces = 0;

        while (spaces < 3 && spaces < line.size() && line[spaces] == ' ')
            ++spaces;
        return line.substr(spaces);
    }

    // Пустой абзац блока не даёт / An empty paragraph gives no block
    void flushParagraph(BlockParser& parser)
    {
        MarkdownBlock block;

        if (parser.paragraph.empty())
            return;
        block.runs = parseInline(parser.paragraph);
        parser.blocks.push_back(block);
        parser.paragraph.clear();
    }

    // Строки абзаца соединяются переводом строки: модели пишут списки и стихи построчно / The paragraph lines are joined with a line break: models write lists and verses line by line
    void textLine(BlockParser& parser, std::string_view line)
    {
        if (headingLine(parser, stripIndent(line)))
            return;
        if (trimSpaces(line).empty())
        {
            flushParagraph(parser);
            return;
        }
        if (!parser.paragraph.empty())
            parser.paragraph += '\n';
        parser.paragraph += listLine(line);
    }
}
