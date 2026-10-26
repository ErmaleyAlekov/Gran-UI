#include "gran/utils/png_writer.hpp"

#include <array>

namespace gran
{
    namespace
    {
        // Таблица CRC-32: остаток каждого байта по многочлену / The CRC-32 table: the remainder of each byte by the polynomial
        std::array<std::uint32_t, 256> buildCrcTable() noexcept
        {
            std::array<std::uint32_t, 256> table{};

            for (std::uint32_t index = 0; index < 256; ++index)
            {
                std::uint32_t value = index;
                for (int bit = 0; bit < 8; ++bit)
                {
                    if ((value & 1u) != 0)
                        value = 0xEDB88320u ^ (value >> 1);
                    else
                        value >>= 1;
                }
                table[index] = value;
            }
            return table;
        }

        // Таблица строится один раз (потокобезопасная инициализация) / The table is built once (thread-safe initialization)
        const std::array<std::uint32_t, 256>& crcTable() noexcept
        {
            static const std::array<std::uint32_t, 256> TABLE = buildCrcTable();
            return TABLE;
        }
    }

    // Табличный CRC-32 по байтам / A table driven byte-wise CRC-32
    std::uint32_t crc32(const std::uint8_t* data, std::size_t size, std::uint32_t crc) noexcept
    {
        const std::array<std::uint32_t, 256>& table = crcTable();
        std::uint32_t value = crc ^ 0xFFFFFFFFu;

        for (std::size_t index = 0; index < size; ++index)
            value = table[(value ^ data[index]) & 0xFFu] ^ (value >> 8);
        return value ^ 0xFFFFFFFFu;
    }

    // Две суммы по модулю 65521 / Two sums modulo 65521
    std::uint32_t adler32(const std::uint8_t* data, std::size_t size) noexcept
    {
        std::uint32_t low = 1;
        std::uint32_t high = 0;

        for (std::size_t index = 0; index < size; ++index)
        {
            low = (low + data[index]) % 65521u;
            high = (high + low) % 65521u;
        }
        return (high << 16) | low;
    }
}
