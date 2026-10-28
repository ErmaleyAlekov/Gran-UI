#include "gran/image/internal/jpeg_internal.hpp"

namespace gran::image::internal
{
    namespace
    {
        // Разность DC: категория по таблице Хаффмана (0–15), затем её биты / The DC difference: the category by the Huffman table (0–15), then its bits
        int dcDifference(const JpegFrame& frame, const JpegComponent& component, JpegReader& reader)
        {
            int size = decodeHuffman(reader, frame.dc[component.dcTable]);

            if (size < 0 || size > 15)
                throwBadImage(*frame.source, "DC code");
            return receiveExtend(reader, size);
        }

        /*
        ** Последовательный блок: DC — предсказатель плюс разность; AC — пары «серия нулей | размер»:
        ** размер 0 и серия 15 — 16 нулей (ZRL), размер 0 и другая серия — конец блока (EOB).
        ** Номер k после серии может выйти за 63 только в испорченных данных — хвост зигзага держит его в блоке.
        **
        ** A sequential block: DC — the predictor plus the difference; AC — the pairs "zero run | size":
        ** size 0 and run 15 — 16 zeros (ZRL), size 0 and another run — the end of the block (EOB).
        ** The number k after a run may pass 63 only in damaged data — the zigzag tail keeps it in the block.
        */
        void decodeBaseline(JpegFrame& frame, JpegComponent& component, JpegReader& reader, std::int16_t* block)
        {
            const HuffmanTable& table = frame.ac[component.acTable];

            component.dcPredictor += dcDifference(frame, component, reader);
            block[0] = static_cast<std::int16_t>(component.dcPredictor);
            for (int k = 1; k < 64;)
            {
                int symbol = decodeHuffman(reader, table);

                if (symbol < 0)
                    throwBadImage(*frame.source, "AC code");
                if ((symbol & 15) == 0 && (symbol >> 4) != 15)
                    break;
                k += symbol >> 4;
                if ((symbol & 15) != 0)
                    block[JPEG_ZIGZAG[k]] = static_cast<std::int16_t>(receiveExtend(reader, symbol & 15));
                k += 1;
            }
        }

        // Первый проход DC: значение со сдвигом на Al / The DC first pass: the value shifted by Al
        void decodeDcFirst(JpegFrame& frame, const JpegScan& scan, JpegComponent& component, JpegReader& reader,
            std::int16_t* block)
        {
            component.dcPredictor += dcDifference(frame, component, reader);
            block[0] = static_cast<std::int16_t>(component.dcPredictor * (1 << scan.approxLow));
        }

        // Уточнение DC: один бит в разряд Al / The DC refinement: one bit into the Al position
        void decodeDcRefine(const JpegScan& scan, JpegReader& reader, std::int16_t* block) noexcept
        {
            if (takeBits(reader, 1) != 0)
                block[0] = static_cast<std::int16_t>(block[0] | (1 << scan.approxLow));
        }
    }

    // Режим блока — по кадру (последовательный / прогрессивный) и параметрам скана / The block mode — by the frame (sequential / progressive) and the scan parameters
    void decodeBlock(JpegFrame& frame, const JpegScan& scan, JpegComponent& component, JpegReader& reader,
        std::int16_t* block)
    {
        if (!frame.progressive)
            decodeBaseline(frame, component, reader, block);
        else if (scan.spectralStart == 0 && scan.approxHigh == 0)
            decodeDcFirst(frame, scan, component, reader, block);
        else if (scan.spectralStart == 0)
            decodeDcRefine(scan, reader, block);
        else if (scan.approxHigh == 0)
            decodeAcFirst(frame, scan, frame.ac[component.acTable], reader, block);
        else
            decodeAcRefine(frame, scan, frame.ac[component.acTable], reader, block);
    }
}
