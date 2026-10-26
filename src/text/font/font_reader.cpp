#include "gran/text/internal/font_parts.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran::text
{
    // Невладеющий взгляд на байты файла / A non-owning view of the file bytes
    FontReader::FontReader(const std::uint8_t* data, std::size_t size) noexcept
        : _data(data), _size(size)
    {
    }

    // Проверка без переполнения суммы offset + length / A check without overflowing offset + length
    bool FontReader::contains(std::size_t offset, std::size_t length) const noexcept
    {
        return offset <= _size && length <= _size - offset;
    }

    // Байт или 0 за границей / A byte or 0 beyond the bounds
    std::uint8_t FontReader::u8(std::size_t offset) const noexcept
    {
        if (!contains(offset, 1))
            return 0;
        return _data[offset];
    }

    // Беззнаковое 16-битное big-endian / An unsigned 16-bit big-endian value
    std::uint16_t FontReader::u16(std::size_t offset) const noexcept
    {
        if (!contains(offset, 2))
            return 0;
        return static_cast<std::uint16_t>((_data[offset] << 8) | _data[offset + 1]);
    }

    // Текст ошибки — из каталога, с именем файла / The error text is from the catalog, with the file name
    void throwBadFont(const std::string& source, const char* reason)
    {
        throw GranException(ErrorCode::FONT, i18n::format(i18n::Message::FONT_BAD_FILE, {source, reason}));
    }
}
