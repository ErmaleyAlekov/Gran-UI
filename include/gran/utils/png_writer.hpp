#ifndef GRAN_UTILS_PNG_WRITER_HPP
#define GRAN_UTILS_PNG_WRITER_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace gran
{
    /*
    ** PNG из пикселей RGBA8 (строки сверху вниз, без выравнивания). Данные не сжимаются:
    ** zlib с блоками «stored» — файл больше, зато кодер в сотню строк и без зависимостей.
    ** Нужен для снимков кадров в тестах и инструменте gran_snapshot.
    **
    ** A PNG from RGBA8 pixels (rows top to bottom, unaligned). The data is not compressed:
    ** zlib with "stored" blocks — the file is larger, but the encoder is a hundred lines without dependencies.
    ** It is needed for frame snapshots in tests and the gran_snapshot tool.
    */
    std::vector<std::uint8_t> encodePng(const std::uint8_t* rgba, std::uint32_t width, std::uint32_t height);
    // Размер буфера в байтах; без него вызывающий гарантирует width × height × 4 байт.
    // Buffer size in bytes; callers omitting it must supply width × height × 4 bytes.
    std::vector<std::uint8_t> encodePng(const std::uint8_t* rgba, std::size_t bytes, std::uint32_t width, std::uint32_t height);

    // Запись PNG в файл; false — файл не записан / Writing a PNG into a file; false — the file is not written
    bool writePng(const std::string& path, const std::uint8_t* rgba, std::uint32_t width, std::uint32_t height);
    bool writePng(const std::string& path, const std::uint8_t* rgba, std::size_t bytes, std::uint32_t width, std::uint32_t height);

    // ---- png_checksums.cpp ----

    // CRC-32 (многочлен 0xEDB88320) с продолжением от crc / CRC-32 (polynomial 0xEDB88320) continuing from crc
    std::uint32_t crc32(const std::uint8_t* data, std::size_t size, std::uint32_t crc = 0) noexcept;

    // Adler-32 для потока zlib / Adler-32 for the zlib stream
    std::uint32_t adler32(const std::uint8_t* data, std::size_t size) noexcept;
}

#endif
