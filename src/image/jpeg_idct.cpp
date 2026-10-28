#include "gran/image/internal/jpeg_internal.hpp"

#include <cmath>

namespace gran::image::internal
{
    namespace
    {
        // Базис обратного ДКП: values[x·8 + u] = c(u)/2 · cos((2x + 1)uπ/16), c(0) = 1/√2 / The inverse DCT basis
        struct Cosines
        {
            float values[64];
        };

        // Расчёт базиса / Computing the basis
        Cosines makeCosines()
        {
            Cosines result{};
            const double pi = 3.14159265358979323846;

            for (int x = 0; x < 8; ++x)
            {
                for (int u = 0; u < 8; ++u)
                {
                    double scale = 0.5;

                    if (u == 0)
                        scale = 0.5 / std::sqrt(2.0);
                    result.values[x * 8 + u] = static_cast<float>(scale * std::cos((2 * x + 1) * u * pi / 16.0));
                }
            }
            return result;
        }

        // Таблица считается один раз (статическая локальная — потокобезопасно) / The table is computed once (a static local is thread-safe)
        const Cosines& cosines()
        {
            static const Cosines table = makeCosines();

            return table;
        }

        /*
        ** Одномерное ДКП строки частот v: коэффициенты × квантование → 8 значений по x.
        ** false — строка нулевая (её вклад в столбцы пропускается).
        **
        ** The one-dimensional DCT of the frequency row v: the coefficients × quantization → 8 values by x.
        ** false — the row is zero (its contribution to the columns is skipped).
        */
        bool idctRow(const std::int16_t* coefficients, const std::uint16_t* quant, float* out) noexcept
        {
            const float* basis = cosines().values;
            float frequencies[8];
            bool nonzero = false;

            for (int u = 0; u < 8; ++u)
            {
                frequencies[u] = static_cast<float>(coefficients[u]) * static_cast<float>(quant[u]);
                nonzero = nonzero || coefficients[u] != 0;
            }
            for (int x = 0; nonzero && x < 8; ++x)
            {
                float sum = 0.0f;

                for (int u = 0; u < 8; ++u)
                    sum += basis[x * 8 + u] * frequencies[u];
                out[x] = sum;
            }
            return nonzero;
        }

        /*
        ** Блок 8×8: строки, затем столбцы по ненулевым строкам; сдвиг уровня +128, округление
        ** и ограничение 0–255 (T.81, A.3.3).
        **
        ** An 8×8 block: the rows, then the columns over the nonzero rows; the level shift +128, rounding
        ** and clamping to 0–255 (T.81, A.3.3).
        */
        void idctBlock(const std::int16_t* coefficients, const std::uint16_t* quant, std::uint8_t* out,
            std::size_t stride) noexcept
        {
            const float* basis = cosines().values;
            float rows[64];
            bool used[8];

            for (int v = 0; v < 8; ++v)
                used[v] = idctRow(coefficients + v * 8, quant + v * 8, rows + v * 8);
            for (int y = 0; y < 8; ++y)
            {
                for (int x = 0; x < 8; ++x)
                {
                    float sum = 128.5f;

                    for (int v = 0; v < 8; ++v)
                    {
                        if (used[v])
                            sum += basis[y * 8 + v] * rows[v * 8 + x];
                    }
                    if (sum < 0.0f)
                        sum = 0.0f;
                    if (sum > 255.0f)
                        sum = 255.0f;
                    out[y * stride + x] = static_cast<std::uint8_t>(sum);
                }
            }
        }
    }

    // Все блоки компоненты → плоскость шириной blocksWide·8; коэффициенты больше не нужны / All the component blocks → a plane of blocksWide·8 wide; the coefficients are no longer needed
    void idctComponent(JpegComponent& component, const std::uint16_t* quant)
    {
        std::size_t stride = static_cast<std::size_t>(component.blocksWide) * 8;

        component.plane.assign(stride * component.blocksHigh * 8, 0);
        for (std::uint32_t row = 0; row < component.blocksHigh; ++row)
        {
            for (std::uint32_t column = 0; column < component.blocksWide; ++column)
            {
                std::size_t block = static_cast<std::size_t>(row) * component.blocksWide + column;

                idctBlock(component.coefficients.data() + block * 64, quant,
                    component.plane.data() + row * 8 * stride + column * 8, stride);
            }
        }
        component.coefficients.clear();
        component.coefficients.shrink_to_fit();
    }
}
