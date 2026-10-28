#include "gran/image/internal/image_error.hpp"
#include "gran/image/image.hpp"

#include <cstring>

namespace gran::image
{
    /*
    ** Формат — по подписи в начале данных, а не по расширению файла: PNG — 8 байт
    ** 89 50 4E 47 0D 0A 1A 0A, JPEG — FF D8 FF (SOI и начало следующего маркера).
    **
    ** The format is by the signature at the data start, not by the file extension: PNG — 8 bytes
    ** 89 50 4E 47 0D 0A 1A 0A, JPEG — FF D8 FF (SOI and the start of the next marker).
    */
    Image decodeImage(const std::uint8_t* data, std::size_t size, const std::string& source)
    {
        static const std::uint8_t PNG_SIGNATURE[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

        if (size >= 8 && std::memcmp(data, PNG_SIGNATURE, 8) == 0)
            return decodePng(data, size, source);
        if (size >= 3 && data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF)
            return decodeJpeg(data, size, source);
        internal::throwBadImage(source, "unknown format");
    }
}
