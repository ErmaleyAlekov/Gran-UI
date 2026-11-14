#ifndef GRAN_TEXT_INTERNAL_FONT_PARTS_HPP
#define GRAN_TEXT_INTERNAL_FONT_PARTS_HPP

#include "gran/text/font_face.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace gran::text
{
    /*
    ** Чтение чисел big-endian из данных шрифта. За границей возвращается 0 — запрос
    ** глифа или кернинга никогда не читает чужую память; структуру таблиц при загрузке
    ** проверяет contains, и там ошибка — исключение.
    **
    ** Reading big-endian numbers from the font data. Beyond the bounds 0 is returned — a glyph
    ** or kerning query never reads foreign memory; at load time contains checks the table
    ** structure, and an error there is an exception.
    */
    class FontReader
    {
    public:
        // ---- font_reader.cpp ----
        FontReader(const std::uint8_t* data, std::size_t size) noexcept;
        bool contains(std::size_t offset, std::size_t length) const noexcept;
        std::uint8_t u8(std::size_t offset) const noexcept;
        std::uint16_t u16(std::size_t offset) const noexcept;

        // ---- font_reader_wide.cpp ----
        std::int16_t i16(std::size_t offset) const noexcept;
        std::uint32_t u32(std::size_t offset) const noexcept;

    private:
        const std::uint8_t* _data;
        std::size_t _size;
    };

    // Испорченный шрифт → GranException(FONT) / A damaged font → GranException(FONT)
    [[noreturn]] void throwBadFont(const std::string& source, const char* reason);

    /*
    ** Сборщик начертания: друг FontFace, заполняет его закрытые поля по шагам загрузки.
    **
    ** The face builder: a friend of FontFace, fills its private fields step by step while loading.
    */
    struct FaceBuilder
    {
        // ---- font_tables.cpp ----

        // Каталог таблиц (TTF или начертание index из TTC) / The table directory (a TTF or the face index of a TTC)
        static void readDirectory(FontFace& face, std::uint32_t index, const std::string& source);

        // head, hhea, maxp: единицы, метрики строки, число глифов, формат loca
        // head, hhea, maxp: the units, the line metrics, the glyph count, the loca format
        static void readHeader(FontFace& face, const std::string& source);

        // ---- font_os2.cpp ----

        // OS/2: насыщенность, курсив, высоты заглавных и строчных / OS/2: the weight, italic, cap and x heights
        static void readOs2(FontFace& face);

        // Лучшая подтаблица cmap: формат 12 (вся Юникод) или 4 (BMP) / The best cmap subtable: format 12 (all Unicode) or 4 (BMP)
        static void selectCmap(FontFace& face, const std::string& source);

        // ---- font_names.cpp ----

        // Семейство и начертание из name / The family and the style from name
        static void readNames(FontFace& face);

        // ---- font_gpos_setup.cpp ----

        // Подтаблицы PairPos признака kern из GPOS (абсолютные смещения) / The PairPos subtables of the kern feature from GPOS (absolute offsets)
        static void collectPairSubtables(FontFace& face);
    };

    // ---- font_names.cpp ----

    // Имя семейства (style = false) или начертания (true) из таблицы name: типографское, затем обычное
    // The family (style = false) or style (true) name from the name table: the typographic one, then the plain one
    std::string readFaceName(const FontReader& reader, const TableSpan& table, bool style);

    // ---- font_cmap.cpp ----

    // Поиск в подтаблице формата 4 / A lookup in a format 4 subtable
    std::uint16_t cmapFormat4(const FontReader& reader, std::uint32_t subtable, char32_t symbol) noexcept;

    // Поиск в подтаблице формата 12 / A lookup in a format 12 subtable
    std::uint16_t cmapFormat12(const FontReader& reader, std::uint32_t subtable, char32_t symbol) noexcept;

    // ---- font_glyf.cpp / font_composite.cpp ----

    // Простой глиф: точки, флаги, координаты / A simple glyph: points, flags, coordinates
    bool readSimpleGlyph(const FontReader& reader, std::uint32_t offset, std::uint32_t end, Outline& outline);

    // Расположение глифа в glyf (смещение и конец); false — глиф пуст / The glyph location in glyf (the offset and the end); false — the glyph is empty
    bool glyphLocation(const FontReader& reader, const FaceTables& tables, bool longLoca, std::uint16_t glyph,
        std::uint32_t& offset, std::uint32_t& end) noexcept;

    // ---- font_component.cpp ----

    // Флаги записи компонента составного глифа (OpenType glyf) / The composite glyph component record flags (OpenType glyf)
    constexpr std::uint16_t COMPONENT_ARGS_ARE_WORDS = 0x0001;
    constexpr std::uint16_t COMPONENT_ARGS_ARE_XY = 0x0002;
    constexpr std::uint16_t COMPONENT_HAS_SCALE = 0x0008;
    constexpr std::uint16_t COMPONENT_MORE_COMPONENTS = 0x0020;
    constexpr std::uint16_t COMPONENT_HAS_XY_SCALE = 0x0040;
    constexpr std::uint16_t COMPONENT_HAS_TWO_BY_TWO = 0x0080;

    // Полный размер записи компонента по её флагам — проверяется до чтения / The full component record size by its flags — checked before reading
    std::size_t componentRecordSize(std::uint16_t flags) noexcept;

    // Смещение компонента; возвращает позицию после него / The component offset; returns the position after it
    std::size_t readComponentOffset(const FontReader& reader, std::size_t position, std::uint16_t flags,
        Affine2D& transform);

    // Матрица компонента (F2Dot14); возвращает позицию после неё / The component matrix (F2Dot14); returns the position after it
    std::size_t readComponentTransform(const FontReader& reader, std::size_t position, std::uint16_t flags,
        Affine2D& transform);

    // ---- font_kern.cpp / font_gpos.cpp / font_coverage.cpp ----

    // Тег таблицы или признака из четырёх символов / A table or feature tag from four characters
    std::uint32_t fontTag(const char* name) noexcept;

    // Кернинг из таблицы kern (формат 0) / The kerning from the kern table (format 0)
    float kernTableValue(const FontReader& reader, const TableSpan& kern, std::uint16_t left, std::uint16_t right) noexcept;

    // Сдвиг пары из подтаблицы PairPos; false — пара не описана / The pair adjustment from a PairPos subtable; false — the pair is not described
    bool pairAdjustment(const FontReader& reader, std::size_t subtable, std::uint16_t left, std::uint16_t right,
        float& value) noexcept;

    // Индекс глифа в таблице покрытия; −1 — не покрыт / The glyph index in a coverage table; −1 — not covered
    int coverageIndex(const FontReader& reader, std::size_t coverage, std::uint16_t glyph) noexcept;

    // Класс глифа в ClassDef (0 — по умолчанию) / The glyph class in a ClassDef (0 — the default)
    std::uint16_t glyphClass(const FontReader& reader, std::size_t classDef, std::uint16_t glyph) noexcept;
}

#endif
