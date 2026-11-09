#include "linux_tests.hpp"
#include <iostream>
#include <string>

namespace linux_tests
{
    // Тестовые ошибки не используют каталог пользовательских сообщений.
    // Test errors do not use the user-facing message catalog.
    void require(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }

    // По умолчанию — lavapipe (CPU): проверка работает на любой машине с Mesa; --hardware — настоящая карта.
    // By default — lavapipe (CPU): the check works on any machine with Mesa; --hardware — a real card.
    bool& hardwareMode()
    {
        static bool hardware = false;
        return hardware;
    }
}

/*
** GPU-тесты отделены от unit; --window требует X11/XWayland, backend работает без DISPLAY.
** --hardware (в любом месте) — видеокарта вместо lavapipe; какая именно — GRAN_ADAPTER (стенд Intel + NVIDIA).
**
** GPU tests are separate from unit tests; --window requires X11/XWayland, backend runs without DISPLAY.
** --hardware (anywhere) — a video card instead of lavapipe; which one — GRAN_ADAPTER (the Intel + NVIDIA stand).
*/
int main(int argc, char* argv[])
{
    try
    {
        bool window = false;
        for (int index = 1; index < argc; ++index)
        {
            std::string argument = argv[index];
            if (argument == "--clipboard-peer") return linux_tests::clipboardPeer();
            if (argument == "--xdnd-peer" && index + 4 < argc) return linux_tests::dropPeer(argv + index + 1);
            if (argument == "--hardware") linux_tests::hardwareMode() = true;
            else if (argument == "--window") window = true;
            else throw std::invalid_argument(argument);
        }
        if (window) linux_tests::window();
        else linux_tests::backend();
        std::cout << "Linux checks passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
