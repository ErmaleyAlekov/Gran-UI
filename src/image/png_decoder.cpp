#include "gran/image/internal/png_internal.hpp"

namespace gran::image
{
    /*
    ** Чанки → распаковка (предел — точный размер строк: больше быть не может) → снятие
    ** фильтров → RGBA8 с предумноженной альфой. Палитровая картинка без PLTE — ошибка.
    **
    ** The chunks → unpacking (the limit is the exact row size: there can be no more) → removing
    ** the filters → premultiplied RGBA8. A palette image without PLTE is an error.
    */
    Image decodePng(const std::uint8_t* data, std::size_t size, const std::string& source)
    {
        internal::PngChunks chunks;
        std::vector<std::uint8_t> raw;
        Image image;

        internal::readChunks(data, size, source, chunks);
        if (chunks.header.width == 0)
            internal::throwBadImage(source, "no IHDR");
        if (chunks.header.colorType == 3 && chunks.palette.empty())
            internal::throwBadImage(source, "no PLTE");
        std::size_t expected = (internal::rowBytes(chunks.header) + 1) * chunks.header.height;
        if (!inflateZlib(chunks.compressed.data(), chunks.compressed.size(), expected, raw) || raw.size() != expected)
            internal::throwBadImage(source, "IDAT");
        if (!internal::unfilter(raw, chunks.header))
            internal::throwBadImage(source, "filter");
        internal::convertRows(chunks, raw, image);
        return image;
    }
}
