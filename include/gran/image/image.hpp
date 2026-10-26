#ifndef GRAN_IMAGE_IMAGE_HPP
#define GRAN_IMAGE_IMAGE_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace gran::image
{
    // Наибольшая сторона и площадь картинки (защита от «бомб» — 128 МБ пикселей)
    // The largest image side and area (a guard against "bombs" — 128 MB of pixels)
    constexpr std::uint32_t MAX_SIDE = 16384;
    constexpr std::uint64_t MAX_PIXELS = 32ull * 1024 * 1024;

    // Картинка RGBA8 с предумноженной альфой, строки сверху вниз / An RGBA8 image with premultiplied alpha, rows top to bottom
    struct Image
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::vector<std::uint8_t> pixels;
    };

    // ---- inflate.cpp ----

    /*
    ** Распаковка потока zlib (RFC 1950/1951): заголовок, блоки deflate, проверка Adler-32.
    ** limit — наибольший размер результата: поток, обещающий больше, считается испорченным.
    ** false — данные испорчены (никакого чтения или записи за границами).
    **
    ** Unpacking a zlib stream (RFC 1950/1951): the header, the deflate blocks, the Adler-32 check.
    ** limit is the largest result size: a stream promising more is considered damaged.
    ** false — the data is damaged (no reading or writing beyond the bounds).
    */
    bool inflateZlib(const std::uint8_t* data, std::size_t size, std::size_t limit, std::vector<std::uint8_t>& output);

    // ---- png_decoder.cpp ----

    // PNG из байтов; ошибка — GranException(IMAGE) с именем source / A PNG from bytes; an error is GranException(IMAGE) with the source name
    Image decodePng(const std::uint8_t* data, std::size_t size, const std::string& source);
}

#endif
