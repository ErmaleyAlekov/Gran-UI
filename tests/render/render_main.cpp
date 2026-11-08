#include "render_tests.hpp"

#include <exception>
#include <iostream>
#include <string>

namespace render_tests
{
    // Неизвестные параметры считаются ошибкой, чтобы опечатка не обновила эталоны.
    // Unknown options fail so a typo cannot update baselines.
    Options parse(int argc, char* argv[])
    {
        Options options;

        for (int index = 1; index < argc; ++index)
        {
            std::string argument = argv[index];
            if (argument == "--update") options.update = true;
            else if (argument == "--d3d11") options.backend = gran::rhi::BackendKind::D3D11;
            else if (argument == "--d3d12") options.backend = gran::rhi::BackendKind::D3D12;
            else if (argument == "--hardware") options.warp = false;
            else if (argument == "--output" && index + 1 < argc) options.output = argv[++index];
            else if (argument == "--tolerance" && index + 1 < argc) options.tolerance = std::stoul(argv[++index]);
            else throw std::invalid_argument(argument);
        }
        if (options.tolerance > 255 || (options.update && (!options.warp || options.backend != gran::rhi::BackendKind::D3D12)))
            throw std::invalid_argument("--update requires D3D12 WARP; tolerance <= 255");
        return options;
    }

    // Каждый кадр и живые проверки независимы; исключение даёт ненулевой код возврата.
    // Each frame and live check is independent; an exception returns a nonzero exit code.
    int run(const Options& options)
    {
        const Case cases[] = {{"primitives", 1}, {"primitives", 1.5f}, {"text_image", 1}, {"glass", 1}};
        unsigned failed = 0;

        for (const Case& test : cases)
        {
            try { if (!runCase(test, options)) ++failed; }
            catch (const std::exception& error) { ++failed; std::cerr << "[FAIL] " << test.name << ": " << error.what() << '\n'; }
        }
        if (!options.update && !runReload(options)) ++failed;
        if (!options.update && !runBackend(options)) ++failed;
        if (!options.update && !runResize(options)) ++failed;
        return static_cast<int>(failed != 0);
    }
}

// Отдельный бинарник не добавляет GPU-зависимость к gran_tests.
// A separate binary does not add a GPU dependency to gran_tests.
int main(int argc, char* argv[])
{
    try { return render_tests::run(render_tests::parse(argc, argv)); }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
