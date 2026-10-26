#include "gran/text/internal/font_parts.hpp"
#include "gran/utils/utf8.hpp"

namespace gran::text
{
    namespace
    {
        // Строка UTF-16BE (платформа Windows) или однобайтовая (Mac Roman как Latin-1) → UTF-8
        // A UTF-16BE string (the Windows platform) or a single-byte one (Mac Roman as Latin-1) → UTF-8
        std::string decodeName(const FontReader& reader, std::size_t offset, std::size_t length, bool wide)
        {
            std::string text;

            for (std::size_t index = 0; !wide && index < length; ++index)
                appendUtf8(text, reader.u8(offset + index));
            for (std::size_t index = 0; wide && index + 1 < length; index += 2)
            {
                char32_t unit = reader.u16(offset + index);
                char32_t low = reader.u16(offset + index + 2);
                if (unit >= 0xD800 && unit <= 0xDBFF && low >= 0xDC00 && low <= 0xDFFF && index + 3 < length)
                {
                    unit = 0x10000 + ((unit - 0xD800) << 10) + (low - 0xDC00);
                    index += 2;
                }
                appendUtf8(text, unit);
            }
            return text;
        }

        /*
        ** Имя с номером nameId: предпочтение Windows + английский (США), затем любой Windows,
        ** затем Mac. Возвращает пустую строку, если имени нет.
        **
        ** A name with the number nameId: the preference is Windows + English (US), then any Windows,
        ** then Mac. Returns an empty string if there is no such name.
        */
        std::string findName(const FontReader& reader, const TableSpan& table, std::uint16_t nameId)
        {
            std::uint16_t count = reader.u16(table.offset + 2);
            std::size_t strings = table.offset + reader.u16(table.offset + 4);
            int bestScore = 0;
            std::string best;

            for (std::uint16_t index = 0; index < count && 6 + (index + 1u) * 12u <= table.length; ++index)
            {
                std::size_t record = table.offset + 6 + static_cast<std::size_t>(index) * 12;
                std::uint16_t platform = reader.u16(record);
                int score = 0;
                if (reader.u16(record + 6) != nameId || (platform != 1 && platform != 3))
                    continue;
                score = 1 + (platform == 3) + (platform == 3 && reader.u16(record + 4) == 0x0409);
                if (score > bestScore)
                {
                    bestScore = score;
                    best = decodeName(reader, strings + reader.u16(record + 10), reader.u16(record + 8), platform == 3);
                }
            }
            return best;
        }
    }

    // Типографское имя (16, 17) точнее обычного (1, 2): «Segoe UI» + «Semibold», а не «Segoe UI Semibold»
    // The typographic name (16, 17) is more precise than the plain one (1, 2): "Segoe UI" + "Semibold", not "Segoe UI Semibold"
    std::string readFaceName(const FontReader& reader, const TableSpan& table, bool style)
    {
        std::uint16_t typographic = 16;
        std::uint16_t plain = 1;

        if (table.length < 6)
            return std::string();
        if (style)
        {
            typographic = 17;
            plain = 2;
        }
        std::string name = findName(reader, table, typographic);
        if (name.empty())
            name = findName(reader, table, plain);
        return name;
    }

    // Имена загружаемого начертания / The names of the face being loaded
    void FaceBuilder::readNames(FontFace& face)
    {
        FontReader reader(face._file->data(), face._file->size());

        face._family = readFaceName(reader, face._tables.name, false);
        face._style = readFaceName(reader, face._tables.name, true);
    }
}
