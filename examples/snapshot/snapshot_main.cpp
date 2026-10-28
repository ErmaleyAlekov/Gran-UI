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

#include <cstdlib>
#include <exception>
#include <iostream>

// Точка входа: 0 — снимок записан, 1 — ошибка разметки, шрифта или GPU
// The entry point: 0 — the snapshot is written, 1 — a markup, font or GPU error
int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cerr << "gran_snapshot <markup.xml> <frame.png> [width height [scale]]\n";
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
