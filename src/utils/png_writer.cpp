#include "gran/utils/png_writer.hpp"

#include <algorithm>
#include <fstream>

namespace gran
{
    namespace
    {
        // Число big-endian в конец буфера / A big-endian number to the buffer end
        void appendU32(std::vector<std::uint8_t>& out, std::uint32_t value)
        {
            out.push_back(static_cast<std::uint8_t>(value >> 24));
            out.push_back(static_cast<std::uint8_t>(value >> 16));
            out.push_back(static_cast<std::uint8_t>(value >> 8));
            out.push_back(static_cast<std::uint8_t>(value));
        }

        // Чанк: длина, тип, данные, CRC типа и данных / A chunk: the length, the type, the data, the CRC of the type and data
        void appendChunk(std::vector<std::uint8_t>& out, const char* type, const std::vector<std::uint8_t>& data)
        {
            std::size_t start = out.size() + 4;

            appendU32(out, static_cast<std::uint32_t>(data.size()));
            out.insert(out.end(), type, type + 4);
            out.insert(out.end(), data.begin(), data.end());
            appendU32(out, crc32(out.data() + start, out.size() - start));
        }

        /*
        ** Поток zlib без сжатия: заголовок 78 01, блоки «stored» до 65535 байт
        ** (байт BFINAL/BTYPE, LEN, NLEN little-endian), в конце Adler-32.
        **
        ** A zlib stream without compression: the header 78 01, "stored" blocks up to 65535 bytes
        ** (the BFINAL/BTYPE byte, LEN, NLEN little-endian), Adler-32 at the end.
        */
        std::vector<std::uint8_t> storeZlib(const std::vector<std::uint8_t>& raw)
        {
            std::vector<std::uint8_t> out{0x78, 0x01};
            std::size_t position = 0;

            do
            {
                std::size_t length = std::min<std::size_t>(65535, raw.size() - position);
                bool last = position + length == raw.size();
                out.push_back(static_cast<std::uint8_t>(last));
                out.push_back(static_cast<std::uint8_t>(length & 0xFF));
                out.push_back(static_cast<std::uint8_t>(length >> 8));
                out.push_back(static_cast<std::uint8_t>(~length & 0xFF));
                out.push_back(static_cast<std::uint8_t>((~length >> 8) & 0xFF));
                out.insert(out.end(), raw.begin() + static_cast<std::ptrdiff_t>(position),
                    raw.begin() + static_cast<std::ptrdiff_t>(position + length));
                position += length;
            } while (position < raw.size());
            appendU32(out, adler32(raw.data(), raw.size()));
            return out;
        }
    }

    // Подпись, IHDR (RGBA, 8 бит), IDAT (строки с фильтром 0), IEND / The signature, IHDR (RGBA, 8 bits), IDAT (rows with filter 0), IEND
    std::vector<std::uint8_t> encodePng(const std::uint8_t* rgba, std::uint32_t width, std::uint32_t height)
    {
        std::vector<std::uint8_t> png{0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
        std::vector<std::uint8_t> header;
        std::vector<std::uint8_t> raw;
        std::size_t rowBytes = static_cast<std::size_t>(width) * 4;

        appendU32(header, width);
        appendU32(header, height);
        header.insert(header.end(), {8, 6, 0, 0, 0});
        appendChunk(png, "IHDR", header);
        raw.reserve((rowBytes + 1) * height);
        for (std::uint32_t row = 0; row < height; ++row)
        {
            raw.push_back(0);
            raw.insert(raw.end(), rgba + row * rowBytes, rgba + (row + 1) * rowBytes);
        }
        appendChunk(png, "IDAT", storeZlib(raw));
        appendChunk(png, "IEND", {});
        return png;
    }

    // Запись байтов PNG / Writing the PNG bytes
    bool writePng(const std::string& path, const std::uint8_t* rgba, std::uint32_t width, std::uint32_t height)
    {
        std::vector<std::uint8_t> png = encodePng(rgba, width, height);
        std::ofstream file(path, std::ios::binary);

        file.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
        return static_cast<bool>(file);
    }
}
