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
}

// GPU-тесты отделены от unit; --window требует X11/XWayland, backend работает без DISPLAY.
// GPU tests are separate from unit tests; --window requires X11/XWayland, backend runs without DISPLAY.
int main(int argc, char* argv[])
{
    try
    {
        if (argc > 1 && std::string(argv[1]) == "--clipboard-peer") return linux_tests::clipboardPeer();
        if (argc > 1 && std::string(argv[1]) == "--window") linux_tests::window();
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
