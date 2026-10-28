#ifndef GRAN_IMAGE_INTERNAL_IMAGE_ERROR_HPP
#define GRAN_IMAGE_INTERNAL_IMAGE_ERROR_HPP

#include <string>

namespace gran::image::internal
{
    // ---- png_chunks.cpp ----

    /*
    ** Ошибка изображения → GranException(IMAGE) с именем источника и технической причиной
    ** (общая для всех декодеров: PNG, JPEG).
    **
    ** An image error → GranException(IMAGE) with the source name and a technical reason
    ** (shared by all the decoders: PNG, JPEG).
    */
    [[noreturn]] void throwBadImage(const std::string& source, const char* reason);
}

#endif
