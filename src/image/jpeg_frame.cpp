#include "gran/image/internal/jpeg_internal.hpp"

#include <algorithm>

namespace gran::image::internal
{
    namespace
    {
        // Деление с округлением вверх / Division rounding up
        std::uint32_t divideUp(std::uint64_t value, std::uint64_t divisor) noexcept
        {
            return static_cast<std::uint32_t>((value + divisor - 1) / divisor);
        }

        // Одна компонента из 3 байт: номер, «h | v», таблица квантования / One component of 3 bytes: the id, "h | v", the quantization table
        void readComponent(JpegFrame& frame, const std::uint8_t* bytes, JpegComponent& component)
        {
            component.id = bytes[0];
            component.h = static_cast<std::uint8_t>(bytes[1] >> 4);
            component.v = static_cast<std::uint8_t>(bytes[1] & 15);
            component.quant = bytes[2];
            if (component.h < 1 || component.h > 4 || component.v < 1 || component.v > 4 || component.quant > 3)
                throwBadImage(*frame.source, "SOF component");
        }
    }

    /*
    ** SOF: точность (только 8 бит), высота, ширина, число компонент (1 или 3), компоненты.
    ** Высота 0 (размер в маркере DNL) и CMYK не поддерживаются.
    **
    ** SOF: the precision (only 8 bits), the height, the width, the component count (1 or 3), the components.
    ** A zero height (the size in the DNL marker) and CMYK are not supported.
    */
    void readFrameHeader(JpegFrame& frame, std::uint8_t marker, const std::uint8_t* segment, std::size_t length)
    {
        std::size_t count = 0;

        if (frame.frameRead || length < 6)
            throwBadImage(*frame.source, "SOF");
        if (segment[0] != 8)
            throwBadImage(*frame.source, "unsupported precision");
        frame.height = readWord(segment + 1);
        frame.width = readWord(segment + 3);
        count = segment[5];
        if ((count != 1 && count != 3) || length < 6 + 3 * count)
            throwBadImage(*frame.source, "unsupported component count");
        frame.componentCount = count;
        for (std::size_t index = 0; index < count; ++index)
            readComponent(frame, segment + 6 + 3 * index, frame.components[index]);
        frame.progressive = marker == 0xC2;
        frame.frameRead = true;
        layoutComponents(frame);
    }

    /*
    ** MCU — наименьший повторяющийся участок: 8·hMax × 8·vMax пикселей. Блоки компоненты
    ** заводятся до границы MCU (так их требует чередующийся скан); видимая часть компоненты —
    ** размер картинки, уменьшенный в hMax/h и vMax/v раз с округлением вверх (T.81, A.1.1).
    **
    ** An MCU is the smallest repeating area: 8·hMax × 8·vMax pixels. The component blocks
    ** are allocated up to the MCU border (an interleaved scan needs them so); the visible part of the component
    ** is the image size reduced hMax/h and vMax/v times rounding up (T.81, A.1.1).
    */
    void layoutComponents(JpegFrame& frame)
    {
        if (frame.width == 0 || frame.height == 0 || frame.width > MAX_SIDE || frame.height > MAX_SIDE
            || static_cast<std::uint64_t>(frame.width) * frame.height > MAX_PIXELS)
            throwBadImage(*frame.source, "size");
        for (std::size_t index = 0; index < frame.componentCount; ++index)
        {
            frame.hMax = std::max(frame.hMax, frame.components[index].h);
            frame.vMax = std::max(frame.vMax, frame.components[index].v);
        }
        frame.mcusWide = divideUp(frame.width, 8u * frame.hMax);
        frame.mcusHigh = divideUp(frame.height, 8u * frame.vMax);
        for (std::size_t index = 0; index < frame.componentCount; ++index)
        {
            JpegComponent& component = frame.components[index];

            component.blocksWide = frame.mcusWide * component.h;
            component.blocksHigh = frame.mcusHigh * component.v;
            component.width = divideUp(static_cast<std::uint64_t>(frame.width) * component.h, frame.hMax);
            component.height = divideUp(static_cast<std::uint64_t>(frame.height) * component.v, frame.vMax);
            component.coefficients.assign(static_cast<std::size_t>(component.blocksWide) * component.blocksHigh * 64, 0);
        }
    }
}
