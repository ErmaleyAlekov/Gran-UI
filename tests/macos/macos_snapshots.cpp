#include "macos_tests.hpp"
#include "../render/render_tests.hpp"
#include "gran/utils/png_writer.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace macos_tests
{
    namespace
    {
        /*
        ** Бюджет сравнения с эталоном D3D12 WARP. Весь кадр — в пределах tolerance на канал; выбросы — не больше 0,1 %
        ** пикселей и не дальше OUTLIER_DELTA. Почему не «ноль отличий», как у D3D11 против D3D12: те два API рисуют одним
        ** растеризатором WARP, а Metal — другим GPU, и покрытие пикселя на самом краю фигуры считается чуть иначе
        ** (первый прогон: 24/15/13 пикселей из 76 800, наибольшее отличие 29/8/30; -fno-fast-math ничего не изменил).
        ** Сдвиг на полпикселя, перевёрнутый кадр или ошибка размытия дают тысячи отличий и в бюджет не попадают.
        **
        ** The comparison budget against the D3D12 WARP baseline. The whole frame is within tolerance per channel; the outliers
        ** are at most 0.1 % of the pixels and not farther than OUTLIER_DELTA. Why not "zero differences" as for D3D11 against
        ** D3D12: those two APIs draw with one WARP rasterizer, while Metal is another GPU, and the coverage of a pixel right on
        ** a shape edge is computed slightly differently (the first run: 24/15/13 pixels of 76,800, the largest difference
        ** 29/8/30; -fno-fast-math changed nothing). A half-pixel shift, a flipped frame or a blur error give thousands of
        ** differences and do not fit the budget.
        */
        constexpr unsigned OUTLIER_DELTA = 48;
        constexpr std::size_t OUTLIER_SHARE = 1000;
        constexpr unsigned PRINTED_PIXELS = 6;

        // Первые отличающиеся пиксели: координаты, факт и эталон — место видно без скачивания артефакта
        // The first differing pixels: the coordinates, the actual and the expected — the place is visible without the artifact
        void printDifferences(const gran::image::Image& actual, const gran::image::Image& expected, unsigned tolerance)
        {
            unsigned printed = 0;

            for (std::size_t offset = 0; offset < actual.pixels.size() && printed < PRINTED_PIXELS; offset += 4)
            {
                unsigned delta = 0;
                for (unsigned channel = 0; channel < 4; ++channel)
                    delta = std::max(delta, static_cast<unsigned>(std::abs(
                        static_cast<int>(actual.pixels[offset + channel]) - expected.pixels[offset + channel])));
                if (delta <= tolerance)
                    continue;
                std::size_t pixel = offset / 4;
                std::cout << "    (" << pixel % actual.width << "," << pixel / actual.width << ") got";
                for (unsigned channel = 0; channel < 4; ++channel)
                    std::cout << ' ' << static_cast<unsigned>(actual.pixels[offset + channel]);
                std::cout << " want";
                for (unsigned channel = 0; channel < 4; ++channel)
                    std::cout << ' ' << static_cast<unsigned>(expected.pixels[offset + channel]);
                std::cout << '\n';
                ++printed;
            }
        }

        // Снимок сцены бэкендом Metal → PNG в options.output → сравнение по бюджету
        // A scene snapshot by the Metal backend → PNG into options.output → the budget comparison
        bool runMetalCase(const render_tests::Case& test, const render_tests::Options& options)
        {
            std::string suffix = std::string(test.name) + "-" + std::to_string(static_cast<int>(test.scale * 100)) + ".png";
            gran::image::Image actual = render_tests::render(test, options);
            gran::image::Image expected = render_tests::read("tests/render/baselines/" + suffix);

            std::filesystem::create_directories(options.output);
            gran::writePng(options.output + "/" + suffix, actual.pixels.data(), actual.width, actual.height);
            auto strict = gran::render::comparePixels(actual, expected, options.tolerance);
            auto loose = gran::render::comparePixels(actual, expected, OUTLIER_DELTA);
            std::size_t budget = static_cast<std::size_t>(actual.width) * actual.height / OUTLIER_SHARE;
            bool passed = strict.sameSize && loose.differentPixels == 0 && strict.differentPixels <= budget;
            if (passed)
                std::cout << "[ OK ] ";
            else
                std::cout << "[FAIL] ";
            std::cout << suffix << " pixels=" << strict.differentPixels << " budget=" << budget << " max=" << strict.maxDelta << '\n';
            if (strict.sameSize)
                printDifferences(actual, expected, options.tolerance);
            return passed;
        }
    }

    // Сцены без текста — по бюджету; text_image — только для сведения: шрифты macOS другие, эталона под них нет
    // The scenes without text — by the budget; text_image is informational only: the macOS fonts differ, there is no baseline for them
    unsigned snapshots(unsigned tolerance)
    {
        const render_tests::Case required[] = {{"primitives", 1}, {"primitives", 1.5f}, {"glass", 1}};
        render_tests::Options options;
        unsigned failed = 0;

        options.backend = gran::rhi::BackendKind::METAL;
        options.warp = false;
        options.tolerance = tolerance;
        for (const render_tests::Case& test : required)
        {
            try { if (!runMetalCase(test, options)) ++failed; }
            catch (const std::exception& error) { ++failed; std::cout << "[FAIL] " << test.name << ": " << error.what() << '\n'; }
        }
        try
        {
            std::cout << "informational, fonts differ: ";
            render_tests::runCase(render_tests::Case{"text_image", 1}, options);
        }
        catch (const std::exception& error) { std::cout << "text_image: " << error.what() << '\n'; }
        return failed;
    }
}
