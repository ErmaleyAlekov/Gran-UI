#include "macos_tests.hpp"

#include <iostream>
#include <string>

/*
** bin/gran_macos_tests [--tolerance N]: проверки бэкенда, окно Cocoa, затем снимки. N — допуск на канал при сравнении
** с эталонами (по умолчанию 2, как у сравнения D3D11 с D3D12 на Windows).
**
** bin/gran_macos_tests [--tolerance N]: the backend checks, the Cocoa window, then the snapshots. N is the per-channel tolerance
** for the baseline comparison (2 by default, as for comparing D3D11 with D3D12 on Windows).
*/
int main(int argc, char* argv[])
{
    try
    {
        unsigned long tolerance = 2;
        for (int index = 1; index < argc; ++index)
        {
            std::string argument = argv[index];
            if (argument == "--tolerance" && index + 1 < argc)
                tolerance = std::stoul(argv[++index]);
            else
                throw std::invalid_argument(argument);
        }
        // Проверка до сужения типа: огромное число не должно превратиться в маленький допуск
        // The check before narrowing: a huge number must not turn into a small tolerance
        macos_tests::require(tolerance <= 255, "tolerance <= 255");
        macos_tests::backend();
        std::cout << "Metal backend checks passed\n";
        macos_tests::window();
        std::cout << "Cocoa window checks passed\n";
        unsigned failed = macos_tests::snapshots(static_cast<unsigned>(tolerance));
        if (failed != 0)
            std::cout << "[FAIL] Metal snapshots: " << failed << '\n';
        return static_cast<int>(failed != 0);
    }
    catch (const std::exception& error)
    {
        std::cout << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
