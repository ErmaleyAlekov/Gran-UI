#include "render_tests.hpp"

#include "gran/utils/mapped_file.hpp"
#include "gran/utils/png_writer.hpp"

#include <filesystem>
#include <iostream>

namespace render_tests
{
    // PNG читается тем же декодером, который используется в приложении.
    // PNG is read by the same decoder used in the application.
    gran::image::Image read(const std::string& path)
    {
        auto file = gran::MappedFile::open(path);
        if (!file)
            throw std::runtime_error("missing baseline: " + path);
        return gran::image::decodePng(file->data(), file->size(), path);
    }

    // Эталон меняется только явным --update; неудачный кадр остаётся для просмотра.
    // A baseline changes only with explicit --update; a failing frame remains for inspection.
    bool runCase(const Case& test, const Options& options)
    {
        std::string suffix = std::string(test.name) + "-" + std::to_string(static_cast<int>(test.scale * 100)) + ".png";
        std::string baseline = "tests/render/baselines/" + suffix;
        gran::image::Image actual = render(test, options);
        std::filesystem::create_directories(options.output);
        gran::writePng(options.output + "/" + suffix, actual.pixels.data(), actual.width, actual.height);
        if (options.update)
        {
            std::filesystem::create_directories("tests/render/baselines");
            gran::writePng(baseline, actual.pixels.data(), actual.width, actual.height);
            std::cout << "[UPDATE] " << suffix << '\n';
            return true;
        }
        auto comparison = gran::render::comparePixels(actual, read(baseline), options.tolerance);
        bool passed = comparison.sameSize && comparison.differentPixels == 0;
        if (passed)
            std::cout << "[ OK ] ";
        else
            std::cout << "[FAIL] ";
        std::cout << suffix << " pixels=" << comparison.differentPixels << " max=" << comparison.maxDelta << '\n';
        return passed;
    }
}
