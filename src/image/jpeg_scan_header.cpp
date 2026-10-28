#include "gran/image/internal/jpeg_internal.hpp"

namespace gran::image::internal
{
    namespace
    {
        // Номер компоненты кадра по её id; нет такой — ошибка / The frame component number by its id; none — an error
        std::uint8_t componentIndex(const JpegFrame& frame, std::uint8_t id)
        {
            for (std::size_t index = 0; index < frame.componentCount; ++index)
            {
                if (frame.components[index].id == id)
                    return static_cast<std::uint8_t>(index);
            }
            throwBadImage(*frame.source, "SOS component");
        }

        /*
        ** Параметры прогрессивного скана (T.81, G.1.1.1.1): DC-скан — только Ss = Se = 0, AC-скан —
        ** одна компонента и 1 ≤ Ss ≤ Se ≤ 63; Al не больше 13. Последовательный скан — Ss/Se не важны.
        **
        ** The progressive scan parameters (T.81, G.1.1.1.1): a DC scan — only Ss = Se = 0, an AC scan —
        ** one component and 1 ≤ Ss ≤ Se ≤ 63; Al is at most 13. A sequential scan — Ss/Se do not matter.
        */
        void checkSpectral(const JpegFrame& frame, const JpegScan& scan)
        {
            bool valid = scan.approxLow <= 13 && scan.spectralEnd <= 63;

            if (!frame.progressive)
                return;
            if (scan.spectralStart == 0 && scan.spectralEnd != 0)
                valid = false;
            if (scan.spectralStart > 0 && (scan.count != 1 || scan.spectralEnd < scan.spectralStart))
                valid = false;
            if (!valid)
                throwBadImage(*frame.source, "SOS spectral");
        }

        // Нужные скану таблицы Хаффмана на месте / The Huffman tables the scan needs are present
        void checkTables(const JpegFrame& frame, const JpegScan& scan)
        {
            bool needDc = !frame.progressive || (scan.spectralStart == 0 && scan.approxHigh == 0);
            bool needAc = !frame.progressive || scan.spectralStart > 0;

            for (std::size_t index = 0; index < scan.count; ++index)
            {
                const JpegComponent& component = frame.components[scan.components[index]];

                if ((needDc && !frame.dc[component.dcTable].present) || (needAc && !frame.ac[component.acTable].present))
                    throwBadImage(*frame.source, "missing DHT");
            }
        }
    }

    /*
    ** SOS: число компонент, для каждой — id и «DC-таблица | AC-таблица», затем Ss, Se, «Ah | Al».
    ** Номера таблиц запоминаются в компоненте (следующий скан может их сменить).
    **
    ** SOS: the component count, for each — the id and "DC table | AC table", then Ss, Se, "Ah | Al".
    ** The table numbers are kept in the component (the next scan may change them).
    */
    void readScanHeader(JpegFrame& frame, const std::uint8_t* segment, std::size_t length, JpegScan& scan)
    {
        if (!frame.frameRead || length < 1 || segment[0] < 1 || segment[0] > frame.componentCount
            || length < 4 + 2 * static_cast<std::size_t>(segment[0]))
            throwBadImage(*frame.source, "SOS");
        scan.count = segment[0];
        for (std::size_t index = 0; index < scan.count; ++index)
        {
            const std::uint8_t* bytes = segment + 1 + 2 * index;
            JpegComponent& component = frame.components[componentIndex(frame, bytes[0])];

            scan.components[index] = componentIndex(frame, bytes[0]);
            component.dcTable = static_cast<std::uint8_t>((bytes[1] >> 4) & 3);
            component.acTable = static_cast<std::uint8_t>(bytes[1] & 3);
        }
        segment += 1 + 2 * scan.count;
        scan.spectralStart = segment[0];
        scan.spectralEnd = segment[1];
        scan.approxHigh = segment[2] >> 4;
        scan.approxLow = segment[2] & 15;
        checkSpectral(frame, scan);
        checkTables(frame, scan);
    }
}
