#include "gran/image/internal/jpeg_internal.hpp"

namespace gran::image
{
    /*
    ** JPEG: сегменты и сканы (коэффициенты всех блоков копятся — так одинаково работают
    ** последовательный и прогрессивный режимы), затем обратное ДКП каждой компоненты
    ** и перевод цвета. Без кадра, без скана или без таблицы квантования компоненты — ошибка.
    **
    ** JPEG: the segments and the scans (the coefficients of all the blocks accumulate — so the sequential
    ** and the progressive modes work alike), then the inverse DCT of every component
    ** and the color conversion. No frame, no scan or no component quantization table is an error.
    */
    Image decodeJpeg(const std::uint8_t* data, std::size_t size, const std::string& source)
    {
        internal::JpegFrame frame;
        Image image;

        frame.source = &source;
        if (size < 4 || data[0] != 0xFF || data[1] != 0xD8)
            internal::throwBadImage(source, "no SOI");
        internal::readSegments(frame, data, size);
        if (!frame.frameRead || !frame.scanRead)
            internal::throwBadImage(source, "no SOF or SOS");
        for (std::size_t index = 0; index < frame.componentCount; ++index)
        {
            internal::JpegComponent& component = frame.components[index];

            if (!frame.quantPresent[component.quant])
                internal::throwBadImage(source, "no DQT");
            internal::idctComponent(component, frame.quant[component.quant]);
        }
        internal::convertJpeg(frame, image);
        return image;
    }
}
