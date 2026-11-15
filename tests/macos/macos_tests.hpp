#ifndef GRAN_MACOS_TESTS_HPP
#define GRAN_MACOS_TESTS_HPP

#include "gran/rhi/backend_factory.hpp"

#include <stdexcept>

/*
** GPU-проверки бэкенда Metal на macOS (make test-render при NATIVE=1, шаг M5): кадр вне экрана, чтение пикселей,
** размытие подложки, смена размера, повторное использование слотов текстур, отказ на неверные данные; затем снимки
** сцен tests/render/scenes против эталонов D3D12 WARP с допуском. Окна здесь нет — оно появится в M6.
**
** The GPU checks of the Metal backend on macOS (make test-render with NATIVE=1, step M5): an offscreen frame, reading
** pixels, the backdrop blur, resizing, reusing texture slots, rejecting invalid data; then snapshots of the
** tests/render/scenes scenes against the D3D12 WARP baselines within a tolerance. There is no window here — it comes in M6.
*/
namespace macos_tests
{
    struct Scene;

    // Условие ложно → исключение с текстом проверки / A false condition → an exception with the check text
    void require(bool condition, const char* message);
    // Проверки бэкенда на синтетической сцене / The backend checks on a synthetic scene
    void backend();
    // Отказы на неверные слоты, области и размеры кадра / Rejections of invalid slots, regions and frame sizes
    void validate(gran::rhi::RenderBackend& backend, gran::rhi::FrameData frame);
    // Освобождённый слот текстуры отдаётся новой текстуре, кадр видит новую / A released texture slot goes to a new texture, the frame sees the new one
    void textures(gran::rhi::RenderBackend& backend, Scene& scene);
    // Пиксель (x, y) кадра равен color (RGBA, R — младший байт) / The frame pixel (x, y) equals color (RGBA, R is the lowest byte)
    void compare(gran::rhi::RenderBackend& backend, unsigned x, unsigned y, std::uint32_t color);
    // Снимки сцен против эталонов; возвращает число провалов / The scene snapshots against the baselines; returns the failure count
    unsigned snapshots(unsigned tolerance);

    // Синтетическая сцена: красный квадрат 16×16 в точке (4, 4) на синем фоне
    // A synthetic scene: a red 16×16 square at (4, 4) on a blue background
    struct Scene
    {
        explicit Scene(gran::rhi::RenderBackend& backend);
        gran::render::GpuClip clip{};
        gran::render::GpuInstance item{};
        gran::rhi::FrameSegment segment{};
        gran::rhi::FrameData frame;
        unsigned atlas = 0;
        unsigned ramp = 0;
    };
}

#endif
