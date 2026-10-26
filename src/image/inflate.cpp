#include "gran/image/image.hpp"
#include "gran/image/internal/inflate_internal.hpp"
#include "gran/utils/png_writer.hpp"

namespace gran::image
{
    namespace
    {
        // Заголовок zlib: метод 8 (deflate), контрольная сумма заголовка, без словаря
        // The zlib header: method 8 (deflate), the header check sum, no dictionary
        bool validHeader(const std::uint8_t* data) noexcept
        {
            unsigned check = data[0] * 256u + data[1];

            return (data[0] & 0x0F) == 8 && check % 31 == 0 && (data[1] & 0x20) == 0;
        }

        // Блоки до последнего; тип 3 зарезервирован / The blocks up to the last one; type 3 is reserved
        bool inflateBlocks(internal::BitReader& reader, internal::InflateOutput& output)
        {
            bool last = false;

            while (!last)
            {
                last = internal::readBits(reader, 1) != 0;
                std::uint32_t type = internal::readBits(reader, 2);
                bool done = false;
                if (type == 0)
                    done = internal::storedBlock(reader, output);
                else if (type == 1)
                    done = internal::fixedBlock(reader, output);
                else if (type == 2)
                    done = internal::dynamicBlock(reader, output);
                if (!done || reader.failed)
                    return false;
            }
            return true;
        }
    }

    // Заголовок, блоки (последние 4 байта — Adler-32), сверка контрольной суммы / The header, the blocks (the last 4 bytes are Adler-32), the check sum match
    bool inflateZlib(const std::uint8_t* data, std::size_t size, std::size_t limit, std::vector<std::uint8_t>& output)
    {
        internal::BitReader reader;
        internal::InflateOutput target{output, limit};

        output.clear();
        if (size < 6 || !validHeader(data))
            return false;
        reader.data = data + 2;
        reader.size = size - 6;
        if (!inflateBlocks(reader, target))
            return false;
        const std::uint8_t* tail = data + size - 4;
        std::uint32_t expected = (static_cast<std::uint32_t>(tail[0]) << 24) | (static_cast<std::uint32_t>(tail[1]) << 16)
            | (static_cast<std::uint32_t>(tail[2]) << 8) | tail[3];
        return expected == adler32(output.data(), output.size());
    }
}
