#include "gran/utils/png_writer.hpp"
#include "gran/utils/file_system.hpp"

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

        constexpr std::size_t BLOCK = 65535;
        using Bytes = std::vector<std::uint8_t>;

        bool validPixels(const std::uint8_t* rgba, std::size_t capacity, std::uint32_t width, std::uint32_t height)
        {
            if (!rgba || !width || !height || width > INT32_MAX || height > INT32_MAX)
                return false;
            return width <= capacity / height / 4;
        }

        // Один поток zlib; каждый IDAT несёт очередной stored-блок.
        // One zlib stream, with one successive stored block per IDAT.
        template<class Sink>
        bool emitPng(const std::uint8_t* rgba, std::uint32_t width, std::uint32_t height, Sink sink)
        {
            Bytes chunk{0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
            Bytes data;
            appendU32(data, width);
            appendU32(data, height);
            data.insert(data.end(), {8, 6, 0, 0, 0});
            appendChunk(chunk, "IHDR", data);
            if (!sink(chunk))
                return false;

            const std::size_t rowBytes = static_cast<std::size_t>(width) * 4;
            const std::uint64_t stride = static_cast<std::uint64_t>(rowBytes) + 1;
            const std::uint64_t total = stride * height;
            std::uint32_t low = 1, high = 0;
            for (std::uint64_t pos = 0; pos < total;)
            {
                const auto length = static_cast<std::uint16_t>(std::min<std::uint64_t>(BLOCK, total - pos));
                const bool last = total - pos == length;
                data.clear();
                if (!pos)
                    data.insert(data.end(), {0x78, 0x01});
                data.push_back(static_cast<std::uint8_t>(last));
                data.push_back(static_cast<std::uint8_t>(length));
                data.push_back(static_cast<std::uint8_t>(length >> 8));
                const auto inverse = static_cast<std::uint16_t>(~length);
                data.push_back(static_cast<std::uint8_t>(inverse));
                data.push_back(static_cast<std::uint8_t>(inverse >> 8));
                const auto start = data.size();
                const auto end = pos + length;
                while (pos < end)
                {
                    const auto column = pos % stride;
                    if (column == 0)
                    {
                        data.push_back(0);
                        ++pos;
                        continue;
                    }
                    const auto size = static_cast<std::size_t>(std::min(end - pos, stride - column));
                    const auto* pixels = rgba + static_cast<std::size_t>(pos / stride) * rowBytes + (column - 1);
                    data.insert(data.end(), pixels, pixels + size);
                    pos += size;
                }
                for (std::size_t i = start; i < data.size(); ++i)
                {
                    low = (low + data[i]) % 65521u;
                    high = (high + low) % 65521u;
                }
                if (last)
                    appendU32(data, (high << 16) | low);
                chunk.clear();
                appendChunk(chunk, "IDAT", data);
                if (!sink(chunk))
                    return false;
            }
            chunk.clear();
            appendChunk(chunk, "IEND", {});
            return sink(chunk);
        }
    }

    std::vector<std::uint8_t> encodePng(const std::uint8_t* rgba, std::size_t bytes,
        std::uint32_t width, std::uint32_t height)
    {
        Bytes png;
        if (!validPixels(rgba, bytes, width, height))
            return png;
        const auto raw = (static_cast<std::uint64_t>(width) * 4 + 1) * height;
        const auto blocks = 1 + (raw - 1) / BLOCK;
        // Подпись, IHDR, IEND и zlib — 51 байт; блок и его IDAT — ещё 17.
        // Signature, IHDR, IEND and zlib take 51 bytes; each block and its IDAT add 17.
        if (raw > png.max_size() || png.max_size() - raw < 51 || blocks > (png.max_size() - raw - 51) / 17)
            return png;
        png.reserve(static_cast<std::size_t>(raw + blocks * 17 + 51));
        emitPng(rgba, width, height, [&](const Bytes& chunk) {
            png.insert(png.end(), chunk.begin(), chunk.end());
            return true;
        });
        return png;
    }

    std::vector<std::uint8_t> encodePng(const std::uint8_t* rgba, std::uint32_t width, std::uint32_t height)
    {
        return encodePng(rgba, SIZE_MAX, width, height);
    }

    bool writePng(const std::string& path, const std::uint8_t* rgba, std::size_t bytes,
        std::uint32_t width, std::uint32_t height)
    {
        if (!validPixels(rgba, bytes, width, height))
            return false;
        std::ofstream file;
        fs::openFile(file, path, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!file || !emitPng(rgba, width, height, [&](const Bytes& chunk) {
            file.write(reinterpret_cast<const char*>(chunk.data()), static_cast<std::streamsize>(chunk.size()));
            return static_cast<bool>(file);
        }))
            return false;
        file.close();
        return static_cast<bool>(file);
    }

    bool writePng(const std::string& path, const std::uint8_t* rgba, std::uint32_t width, std::uint32_t height)
    {
        return writePng(path, rgba, SIZE_MAX, width, height);
    }
}
