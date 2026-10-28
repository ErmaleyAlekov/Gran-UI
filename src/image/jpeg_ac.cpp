#include "gran/image/internal/jpeg_internal.hpp"

namespace gran::image::internal
{
    namespace
    {
        // Символ AC «серия | размер»; испорченный код — ошибка / An AC symbol "run | size"; a damaged code is an error
        int acSymbol(const JpegFrame& frame, const HuffmanTable& table, JpegReader& reader)
        {
            int symbol = decodeHuffman(reader, table);

            if (symbol < 0)
                throwBadImage(*frame.source, "AC code");
            return symbol;
        }

        // Бит уточнения ненулевого коэффициента: 1 — добавить разряд bit по знаку / The refinement bit of a nonzero coefficient: 1 — add the bit position by the sign
        void refineCoefficient(std::int16_t& coefficient, JpegReader& reader, int bit) noexcept
        {
            if (takeBits(reader, 1) == 0 || (coefficient & bit) != 0)
                return;
            if (coefficient >= 0)
                coefficient = static_cast<std::int16_t>(coefficient + bit);
            else
                coefficient = static_cast<std::int16_t>(coefficient - bit);
        }

        /*
        ** Проход по полосе от k: ненулевые коэффициенты получают бит уточнения, нулевые
        ** отсчитываются; остановка — на нуле номер run + 1 (туда встанет новое значение) или за концом полосы.
        **
        ** A pass over the band from k: the nonzero coefficients get a refinement bit, the zero ones
        ** are counted; it stops at zero number run + 1 (the new value goes there) or past the band end.
        */
        int skipZeros(std::int16_t* block, JpegReader& reader, int k, int end, int run, int bit) noexcept
        {
            while (k <= end)
            {
                std::int16_t& coefficient = block[JPEG_ZIGZAG[k]];

                if (coefficient != 0)
                    refineCoefficient(coefficient, reader, bit);
                else if (run == 0)
                    break;
                else
                    run -= 1;
                k += 1;
            }
            return k;
        }
    }

    /*
    ** Первый проход AC (T.81, G.1.2.2): блок внутри серии EOB пропускается целиком; иначе пары
    ** «серия | размер»; размер 0 и серия r < 15 — серия EOB длиной 2^r + r бит (этот блок — первый в ней).
    **
    ** The AC first pass (T.81, G.1.2.2): a block inside an EOB run is skipped entirely; otherwise the pairs
    ** "run | size"; size 0 and run r < 15 — an EOB run of 2^r + r bits long (this block is the first of it).
    */
    void decodeAcFirst(JpegFrame& frame, const JpegScan& scan, const HuffmanTable& table, JpegReader& reader,
        std::int16_t* block)
    {
        if (frame.eobRun > 0)
        {
            frame.eobRun -= 1;
            return;
        }
        for (int k = scan.spectralStart; k <= scan.spectralEnd;)
        {
            int symbol = acSymbol(frame, table, reader);
            int run = symbol >> 4;

            if ((symbol & 15) == 0 && run < 15)
            {
                frame.eobRun = (1u << run) - 1 + takeBits(reader, run);
                return;
            }
            k += run;
            if ((symbol & 15) != 0)
                block[JPEG_ZIGZAG[k]] = static_cast<std::int16_t>(
                    receiveExtend(reader, symbol & 15) * (1 << scan.approxLow));
            k += 1;
        }
    }

    /*
    ** Уточнение AC (T.81, G.1.2.3; как decode_mcu_AC_refine в libjpeg): новый коэффициент — всегда ±1 << Al
    ** (знак — следующий бит), перед ним — серия нулей, по пути уже ненулевые получают биты уточнения.
    ** Внутри серии EOB уточняются только прежние ненулевые до конца полосы.
    **
    ** The AC refinement (T.81, G.1.2.3; as decode_mcu_AC_refine in libjpeg): a new coefficient is always ±1 << Al
    ** (the sign is the next bit), a zero run goes before it, the already nonzero ones on the way get refinement bits.
    ** Inside an EOB run only the former nonzero ones up to the band end are refined.
    */
    void decodeAcRefine(JpegFrame& frame, const JpegScan& scan, const HuffmanTable& table, JpegReader& reader,
        std::int16_t* block)
    {
        int bit = 1 << scan.approxLow;
        int k = scan.spectralStart;

        for (; frame.eobRun == 0 && k <= scan.spectralEnd; ++k)
        {
            int symbol = acSymbol(frame, table, reader);
            int value = 0;

            if ((symbol & 15) != 0)
                value = bit;
            if ((symbol & 15) != 0 && takeBits(reader, 1) == 0)
                value = -bit;
            if ((symbol & 15) == 0 && (symbol >> 4) < 15)
            {
                frame.eobRun = (1u << (symbol >> 4)) + takeBits(reader, symbol >> 4);
                break;
            }
            k = skipZeros(block, reader, k, scan.spectralEnd, symbol >> 4, bit);
            if (value != 0)
                block[JPEG_ZIGZAG[k]] = static_cast<std::int16_t>(value);
        }
        if (frame.eobRun == 0)
            return;
        for (; k <= scan.spectralEnd; ++k)
        {
            if (block[JPEG_ZIGZAG[k]] != 0)
                refineCoefficient(block[JPEG_ZIGZAG[k]], reader, bit);
        }
        frame.eobRun -= 1;
    }
}
