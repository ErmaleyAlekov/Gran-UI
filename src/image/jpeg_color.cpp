#include "gran/image/internal/jpeg_internal.hpp"

#include <algorithm>

namespace gran::image::internal
{
    namespace
    {
        // Число → байт с округлением и ограничением / A number → a byte with rounding and clamping
        std::uint8_t toByte(float value) noexcept
        {
            value = std::min(255.0f, std::max(0.0f, value + 0.5f));
            return static_cast<std::uint8_t>(value);
        }

        /*
        ** Значение компоненты в пикселе (x, y) картинки. Полная выборка — прямо из плоскости;
        ** уменьшенная — билинейно по центрам отсчётов (для 2× это «треугольный» фильтр libjpeg,
        ** как в браузерах), края — повтор крайнего отсчёта.
        **
        ** The component value at the image pixel (x, y). A full sampling — straight from the plane;
        ** a reduced one — bilinearly by the sample centers (for 2× it is the libjpeg "triangle" filter,
        ** as in browsers), the edges repeat the outermost sample.
        */
        float sampleAt(const JpegFrame& frame, const JpegComponent& component, std::uint32_t x, std::uint32_t y) noexcept
        {
            std::size_t stride = static_cast<std::size_t>(component.blocksWide) * 8;
            const std::uint8_t* plane = component.plane.data();

            if (component.h == frame.hMax && component.v == frame.vMax)
                return plane[y * stride + x];
            float fx = std::max(0.0f, std::min((x + 0.5f) * component.h / frame.hMax - 0.5f, component.width - 1.0f));
            float fy = std::max(0.0f, std::min((y + 0.5f) * component.v / frame.vMax - 0.5f, component.height - 1.0f));
            std::size_t x0 = static_cast<std::size_t>(fx);
            std::size_t y0 = static_cast<std::size_t>(fy);
            std::size_t x1 = std::min<std::size_t>(x0 + 1, component.width - 1);
            std::size_t y1 = std::min<std::size_t>(y0 + 1, component.height - 1);
            float tx = fx - static_cast<float>(x0);
            float ty = fy - static_cast<float>(y0);
            float top = plane[y0 * stride + x0] * (1.0f - tx) + plane[y0 * stride + x1] * tx;
            float bottom = plane[y1 * stride + x0] * (1.0f - tx) + plane[y1 * stride + x1] * tx;
            return top * (1.0f - ty) + bottom * ty;
        }

        // Компоненты — уже RGB: APP14 с преобразованием 0 или id компонент 'R', 'G', 'B' / The components are RGB already: APP14 with transform 0 or the component ids 'R', 'G', 'B'
        bool storedAsRgb(const JpegFrame& frame) noexcept
        {
            if (frame.adobeTransform == 0)
                return true;
            return frame.components[0].id == 'R' && frame.components[1].id == 'G' && frame.components[2].id == 'B';
        }

        /*
        ** Пиксель: серый — три одинаковых канала; YCbCr → RGB по JFIF (ITU-R BT.601, полный диапазон);
        ** альфа всегда 255 (предумножение ничего не меняет).
        **
        ** A pixel: gray — three equal channels; YCbCr → RGB by JFIF (ITU-R BT.601, the full range);
        ** the alpha is always 255 (premultiplication changes nothing).
        */
        void writePixel(const JpegFrame& frame, bool rgb, std::uint32_t x, std::uint32_t y, std::uint8_t* out) noexcept
        {
            float first = sampleAt(frame, frame.components[0], x, y);

            out[0] = toByte(first);
            out[1] = out[0];
            out[2] = out[0];
            out[3] = 255;
            if (frame.componentCount == 1)
                return;
            float second = sampleAt(frame, frame.components[1], x, y);
            float third = sampleAt(frame, frame.components[2], x, y);
            if (rgb)
            {
                out[1] = toByte(second);
                out[2] = toByte(third);
                return;
            }
            out[0] = toByte(first + 1.402f * (third - 128.0f));
            out[1] = toByte(first - 0.344136f * (second - 128.0f) - 0.714136f * (third - 128.0f));
            out[2] = toByte(first + 1.772f * (second - 128.0f));
        }
    }

    // Строки сверху вниз, 4 байта на пиксель / The rows from top to bottom, 4 bytes per pixel
    void convertJpeg(const JpegFrame& frame, Image& image)
    {
        bool rgb = frame.componentCount == 3 && storedAsRgb(frame);

        image.width = frame.width;
        image.height = frame.height;
        image.pixels.assign(static_cast<std::size_t>(frame.width) * frame.height * 4, 0);
        for (std::uint32_t y = 0; y < frame.height; ++y)
        {
            std::uint8_t* row = image.pixels.data() + static_cast<std::size_t>(y) * frame.width * 4;

            for (std::uint32_t x = 0; x < frame.width; ++x)
                writePixel(frame, rgb, x, y, row + static_cast<std::size_t>(x) * 4);
        }
    }
}
