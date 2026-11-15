#include "macos_tests.hpp"
#include "../render/render_tests.hpp"

#include <iostream>

namespace macos_tests
{
    // Сцены без текста сверяются с эталонами D3D12 WARP. text_image — только для сведения: шрифты macOS другие,
    // её эталон появится вместе со своими снимками macOS. Снимки — в scratchpad/render-tests (артефакт CI).
    // The scenes without text are compared with the D3D12 WARP baselines. text_image is informational only: the macOS
    // fonts differ, its baseline comes with own macOS snapshots. The snapshots go to scratchpad/render-tests (a CI artifact).
    unsigned snapshots(unsigned tolerance)
    {
        const render_tests::Case required[] = {{"primitives", 1}, {"primitives", 1.5f}, {"glass", 1}};
        const render_tests::Case informational{"text_image", 1};
        render_tests::Options options;
        unsigned failed = 0;

        options.backend = gran::rhi::BackendKind::METAL;
        options.warp = false;
        options.tolerance = tolerance;
        for (const render_tests::Case& test : required)
        {
            try { if (!render_tests::runCase(test, options)) ++failed; }
            catch (const std::exception& error) { ++failed; std::cout << "[FAIL] " << test.name << ": " << error.what() << '\n'; }
        }
        try
        {
            std::cout << "informational, fonts differ: ";
            render_tests::runCase(informational, options);
        }
        catch (const std::exception& error) { std::cout << "text_image: " << error.what() << '\n'; }
        return failed;
    }
}
