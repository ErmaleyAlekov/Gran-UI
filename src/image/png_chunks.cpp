#include "gran/image/internal/png_internal.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/utils/png_writer.hpp"

#include <cstring>

namespace gran::image::internal
{
    namespace
    {
        // Число big-endian / A big-endian number
        std::uint32_t readU32(const std::uint8_t* data) noexcept
        {
            return (static_cast<std::uint32_t>(data[0]) << 24) | (static_cast<std::uint32_t>(data[1]) << 16)
                | (static_cast<std::uint32_t>(data[2]) << 8) | data[3];
        }

        /*
        ** IHDR: размеры в пределах, допустимое сочетание типа цвета и глубины (таблица PNG),
        ** метод сжатия и фильтрации 0. Чересстрочные (Adam7) пока не поддерживаются.
        **
        ** IHDR: the sizes within the limits, an allowed color type and depth combination (the PNG table),
        ** compression and filter method 0. Interlaced (Adam7) images are not supported yet.
        */
        void readHeader(const std::uint8_t* data, std::uint32_t length, const std::string& source, PngHeader& header)
        {
            std::uint8_t type = 0;
            std::uint8_t depth = 0;

            if (length != 13)
                throwBadImage(source, "IHDR");
            header = PngHeader{readU32(data), readU32(data + 4), data[8], data[9], data[12]};
            type = header.colorType;
            depth = header.depth;
            bool valid = (type == 0 && (depth == 1 || depth == 2 || depth == 4 || depth == 8 || depth == 16))
                || (type == 3 && (depth == 1 || depth == 2 || depth == 4 || depth == 8))
                || ((type == 2 || type == 4 || type == 6) && (depth == 8 || depth == 16));
            if (!valid || data[10] != 0 || data[11] != 0)
                throwBadImage(source, "IHDR type/depth");
            if (header.interlace != 0)
                throwBadImage(source, "interlaced (Adam7) PNG is not supported yet");
            if (header.width == 0 || header.height == 0 || header.width > MAX_SIDE || header.height > MAX_SIDE
                || static_cast<std::uint64_t>(header.width) * header.height > MAX_PIXELS)
                throwBadImage(source, "size");
        }
    }

    // Текст ошибки — из каталога / The error text is from the catalog
    void throwBadImage(const std::string& source, const char* reason)
    {
        throw GranException(ErrorCode::IMAGE, i18n::format(i18n::Message::IMAGE_BAD_FILE, {source, reason}));
    }

    // Чанки по порядку до IEND; у каждого проверяются границы и CRC / The chunks in order up to IEND; each has its bounds and CRC checked
    void readChunks(const std::uint8_t* data, std::size_t size, const std::string& source, PngChunks& chunks)
    {
        static const std::uint8_t SIGNATURE[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
        std::size_t position = 8;

        if (size < 8 || std::memcmp(data, SIGNATURE, 8) != 0)
            throwBadImage(source, "signature");
        while (position + 12 <= size)
        {
            std::uint32_t length = readU32(data + position);
            const std::uint8_t* type = data + position + 4;
            const std::uint8_t* body = type + 4;
            if (length > size - position - 12 || readU32(body + length) != crc32(type, length + 4))
                throwBadImage(source, "chunk");
            if (std::memcmp(type, "IHDR", 4) == 0)
                readHeader(body, length, source, chunks.header);
            else if (std::memcmp(type, "PLTE", 4) == 0)
                chunks.palette.assign(body, body + length);
            else if (std::memcmp(type, "tRNS", 4) == 0)
                chunks.transparency.assign(body, body + length);
            else if (std::memcmp(type, "IDAT", 4) == 0)
                chunks.compressed.insert(chunks.compressed.end(), body, body + length);
            else if (std::memcmp(type, "IEND", 4) == 0)
                return;
            position += 12 + length;
        }
        throwBadImage(source, "no IEND");
    }
}
