#include "render_tests.hpp"
#include "gran/app/app_driver.hpp"
#include "gran/core/gran_exception.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

namespace render_tests
{
    namespace
    {
        // Пиксель RGBA близок к цвету (допуск на сглаживание и округление sRGB) / An RGBA pixel is close to a colour (a tolerance for antialiasing and sRGB rounding)
        bool near(const std::vector<std::uint8_t>& rgba, std::uint32_t width, std::uint32_t x, std::uint32_t y,
            const std::uint8_t (&color)[3])
        {
            std::size_t at = (static_cast<std::size_t>(y) * width + x) * 4;

            for (std::size_t channel = 0; channel < 3; ++channel)
            {
                int diff = static_cast<int>(rgba[at + channel]) - static_cast<int>(color[channel]);
                if (diff < -3 || diff > 3)
                    return false;
            }
            return true;
        }

        /*
        ** Красное окно 64×48 с зелёным квадратом 20×20 от (10, 10): кадр вне экрана должен совпасть по размеру
        ** и цветам в двух точках. scale — масштаб окна (размер кадра растёт, точки берутся в пикселях устройства).
        **
        ** A red 64×48 window with a green 20×20 square from (10, 10): the offscreen frame must match by size
        ** and colours at two points. scale is the window scale (the frame size grows, the points are in device pixels).
        */
        bool checkFrame(gran::AppDriver& driver, unsigned scale)
        {
            const std::uint8_t red[3] = {255, 0, 0};
            const std::uint8_t green[3] = {0, 255, 0};
            std::vector<std::uint8_t> rgba;
            std::uint32_t width = 0;
            std::uint32_t height = 0;

            if (!driver.readFrame(0, rgba, width, height) || width != 64 * scale || height != 48 * scale)
            {
                std::cout << "[FAIL] headless: frame size " << width << "x" << height << "\n";
                return false;
            }
            if (!near(rgba, width, 2 * scale, 2 * scale, red) || !near(rgba, width, 20 * scale, 20 * scale, green))
            {
                std::cout << "[FAIL] headless: frame colours at scale " << scale << "\n";
                return false;
            }
            return true;
        }
    }

    /*
    ** Безоконное окно + бэкенд вне экрана выбранного API (шаг M2): приложение рисует как обычно, тест читает
    ** кадр через AppDriver::readFrame, затем меняет масштаб окна — кадр пересоздаётся в новом размере.
    **
    ** A windowless window + an offscreen backend of the chosen API (step M2): the application draws as usual, the test reads
    ** the frame via AppDriver::readFrame, then changes the window scale — the frame is recreated in the new size.
    */
    bool runHeadless(const Options& options)
    {
        gran::AppOptions appOptions;

        appOptions.headless = true;
        appOptions.backend = options.backend;
        appOptions.forceWarp = options.warp;
        gran::Application app(appOptions);
        app.loadString(R"(<Window width="64" height="48" background="#ff0000"><Canvas>)"
            R"(<Box left="10" top="10" width="20" height="20" background="#00ff00"/></Canvas></Window>)");
        gran::AppDriver driver(app);
        driver.start();
        if (!checkFrame(driver, 1))
            return false;
        driver.window(0).changeScale(2.0f);
        driver.settle();
        if (!checkFrame(driver, 2))
            return false;
        std::cout << "[ OK ] headless: offscreen frame through AppDriver\n";
        return true;
    }
}
