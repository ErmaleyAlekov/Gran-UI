#ifndef GRAN_TESTS_RENDER_TESTS_HPP
#define GRAN_TESTS_RENDER_TESTS_HPP

#include "gran/render/pixel_compare.hpp"
#include "gran/rhi/render_backend.hpp"

#include <string>

namespace render_tests
{
    // Параметры отдельного набора GPU-тестов; обычные unit-тесты не требуют GPU.
    // Options for the separate GPU suite; ordinary unit tests do not require a GPU.
    struct Options
    {
        gran::rhi::BackendKind backend = gran::rhi::BackendKind::D3D12;
        bool warp = true;
        bool update = false;
        unsigned tolerance = 2;
        std::string output = "scratchpad/render-tests";
    };

    // Фиксированные сцены и масштаб позволяют повторять сравнение.
    // Fixed scenes and scale make comparison reproducible.
    struct Case
    {
        const char* name;
        float scale;
    };

    Options parse(int argc, char* argv[]);
    gran::image::Image render(const Case& test, const Options& options);
    gran::image::Image read(const std::string& path);
    bool runCase(const Case& test, const Options& options);
    int run(const Options& options);
    // Живой цикл проверяет замену из таймера и восстановление после неверного файла.
    // The live loop checks timer-triggered replacement and recovery from an invalid file.
    bool runReload(const Options& options);
    bool runBackend(const Options& options);
    // Серия изменений размера в модальном цикле не должна вешать окно (Т-14).
    // A series of size changes in the modal loop must not freeze the window (T-14).
    bool runResize(const Options& options);
}

#endif
