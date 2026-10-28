#include "gran/image/internal/jpeg_internal.hpp"

namespace gran::image::internal
{
    namespace
    {
        // Блок (column, row) компоненты / The (column, row) block of a component
        std::int16_t* blockAt(JpegComponent& component, std::uint32_t column, std::uint32_t row) noexcept
        {
            std::size_t index = static_cast<std::size_t>(row) * component.blocksWide + column;

            return component.coefficients.data() + index * 64;
        }

        /*
        ** Следующий MCU, начиная с unit: перед кратным интервалу — перезапуск (биты, предсказатели DC,
        ** серия EOB). Если настоящие данные кончились: на маркере RST — переход к началу следующего
        ** интервала (блоки до него — хвост серии EOB или испорчены), иначе — total, скан окончен; остальные
        ** блоки не трогаются (как в libjpeg). Так стоимость скана ограничена его данными: пустой скан
        ** большой картинки не проходит по всем её блокам.
        **
        ** The next MCU starting from unit: before a multiple of the interval — a restart (the bits, the DC predictors,
        ** the EOB run). If the real data is over: at an RST marker — moving to the start of the next
        ** interval (the blocks before it are an EOB run tail or damaged), otherwise — total, the scan is over; the remaining
        ** blocks are left alone (as in libjpeg). So the scan cost is bounded by its data: an empty scan
        ** of a large image does not pass over all its blocks.
        */
        std::uint32_t nextUnit(JpegFrame& frame, JpegReader& reader, std::uint32_t unit, std::uint32_t total) noexcept
        {
            std::uint32_t interval = frame.restartInterval;

            while (unit < total)
            {
                if (interval != 0 && unit != 0 && unit % interval == 0)
                {
                    restartBits(reader);
                    for (std::size_t index = 0; index < frame.componentCount; ++index)
                        frame.components[index].dcPredictor = 0;
                    frame.eobRun = 0;
                }
                fillBits(reader);
                if (reader.count > reader.padding)
                    return unit;
                if (interval == 0 || !reader.marker || reader.pos + 1 >= reader.size
                    || reader.data[reader.pos + 1] < 0xD0 || reader.data[reader.pos + 1] > 0xD7)
                    return total;
                unit = (unit / interval + 1) * interval;
            }
            return total;
        }

        /*
        ** Чередующийся скан: MCU слева направо, сверху вниз; в MCU — для каждой компоненты
        ** скана её h × v блоков по строкам.
        **
        ** An interleaved scan: MCUs from left to right, top to bottom; in an MCU — for every component
        ** of the scan its h × v blocks by rows.
        */
        void decodeInterleaved(JpegFrame& frame, const JpegScan& scan, JpegReader& reader)
        {
            std::uint32_t total = frame.mcusWide * frame.mcusHigh;

            for (std::uint32_t unit = nextUnit(frame, reader, 0, total); unit < total;
                unit = nextUnit(frame, reader, unit + 1, total))
            {
                std::uint32_t mcuX = unit % frame.mcusWide;
                std::uint32_t mcuY = unit / frame.mcusWide;

                for (std::size_t index = 0; index < scan.count; ++index)
                {
                    JpegComponent& component = frame.components[scan.components[index]];

                    for (std::uint32_t v = 0; v < component.v; ++v)
                    {
                        for (std::uint32_t h = 0; h < component.h; ++h)
                            decodeBlock(frame, scan, component, reader,
                                blockAt(component, mcuX * component.h + h, mcuY * component.v + v));
                    }
                }
            }
        }

        /*
        ** Одиночный скан: блоки только видимой части компоненты (ceil(width/8) × ceil(height/8)),
        ** «MCU» — один блок (T.81, A.2.2).
        **
        ** A non-interleaved scan: the blocks of the visible component part only (ceil(width/8) × ceil(height/8)),
        ** an "MCU" is one block (T.81, A.2.2).
        */
        void decodeSingle(JpegFrame& frame, const JpegScan& scan, JpegReader& reader)
        {
            JpegComponent& component = frame.components[scan.components[0]];
            std::uint32_t wide = (component.width + 7) / 8;
            std::uint32_t high = (component.height + 7) / 8;

            for (std::uint32_t unit = nextUnit(frame, reader, 0, wide * high); unit < wide * high;
                unit = nextUnit(frame, reader, unit + 1, wide * high))
            {
                decodeBlock(frame, scan, component, reader, blockAt(component, unit % wide, unit / wide));
            }
        }
    }

    /*
    ** Скан: заголовок, затем энтропийные данные с начала после заголовка. Предсказатели DC
    ** и серия EOB обнуляются в начале каждого скана. Возвращается позиция, где чтение
    ** остановилось (маркер после данных или конец файла).
    **
    ** A scan: the header, then the entropy data from the start after the header. The DC predictors
    ** and the EOB run are reset at the start of every scan. Returns the position where reading
    ** stopped (the marker after the data or the end of the file).
    */
    std::size_t readScan(JpegFrame& frame, const std::uint8_t* data, std::size_t size, std::size_t pos)
    {
        std::size_t length = readWord(data + pos - 2) - 2;
        JpegScan scan;
        JpegReader reader;

        readScanHeader(frame, data + pos, length, scan);
        reader.data = data;
        reader.size = size;
        reader.pos = pos + length;
        for (std::size_t index = 0; index < frame.componentCount; ++index)
            frame.components[index].dcPredictor = 0;
        frame.eobRun = 0;
        if (scan.count == 1)
            decodeSingle(frame, scan, reader);
        else
            decodeInterleaved(frame, scan, reader);
        frame.scanRead = true;
        return reader.pos;
    }
}
