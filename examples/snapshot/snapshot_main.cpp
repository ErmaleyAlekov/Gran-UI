/*
** gran_snapshot — кадр разметки без окна: разбор и проверка схемой, стили, компоновка,
** отрисовка D3D12 вне экрана, PNG. Нужен для визуальных проверок и тестов-эталонов.
** Вызов: gran_snapshot <разметка.xml> <кадр.png> [ширина высота [масштаб]]
**
** gran_snapshot — a markup frame without a window: parsing with the schema check, styles, layout,
** offscreen D3D12 rendering, PNG. It is needed for visual checks and reference tests.
** Usage: gran_snapshot <markup.xml> <frame.png> [width height [scale]]
*/
#include "snapshot.hpp"

#include "gran/utils/program_arguments.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

// Точка входа: 0 — снимок записан, 1 — ошибка разметки, шрифта или GPU
// The entry point: 0 — the snapshot is written, 1 — a markup, font or GPU error
// Аргументы — в UTF-8 (gran::programArguments): на Windows argv в ANSI теряет «日本» в путях (Т-56)
// The arguments are in UTF-8 (gran::programArguments): on Windows the ANSI argv loses "日本" in paths (T-56)
int main(int argc, char* argv[])
{
    std::vector<std::string> arguments = gran::programArguments(argc, argv);
    std::vector<char*> utf8;

    for (std::string& argument : arguments)
        utf8.push_back(&argument[0]);
    argc = static_cast<int>(utf8.size());
    argv = utf8.data();
    if (argc < 3)
    {
        std::cerr << "gran_snapshot <markup.xml> <frame.png> [width height [scale]] "
                     "[--d3d11|--d3d12|--vulkan|--metal] [--software|--warp]\n";
        return 1;
    }
    try
    {
        return snapshot::renderSnapshot(snapshot::parseArguments(argc, argv));
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
