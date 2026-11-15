#ifndef GRAN_MACOS_TESTS_HPP
#define GRAN_MACOS_TESTS_HPP

#include "gran/rhi/backend_factory.hpp"
#include "gran/platform/window.hpp"

#include <stdexcept>
#include <vector>

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#endif

/*
** GPU-проверки бэкенда Metal на macOS (make test-render при NATIVE=1, шаг M5): кадр вне экрана, чтение пикселей,
** размытие подложки, смена размера, повторное использование слотов текстур, отказ на неверные данные; затем снимки
** сцен tests/render/scenes против эталонов D3D12 WARP с допуском.
**
** The GPU checks of the Metal backend on macOS (make test-render with NATIVE=1, step M5): an offscreen frame, reading
** pixels, the backdrop blur, resizing, reusing texture slots, rejecting invalid data; then snapshots of the
** tests/render/scenes scenes against the D3D12 WARP baselines within a tolerance.
**
** Окно Cocoa (M6, macos_window.mm и др. — Objective-C++): настоящее окно с CAMetalLayer, кадры, чтение кадра бэкендом,
** ввод через очередь AppKit, смена размера, буфер обмена, wake и закрытие.
**
** The Cocoa window (M6, macos_window.mm and others — Objective-C++): a real window with CAMetalLayer, frames, reading the
** frame by the backend, input through the AppKit queue, resizing, the clipboard, wake and closing.
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
    // Окно Cocoa с Metal (macos_window.mm), ввод и закрытие (macos_input.mm)
    // The Cocoa window with Metal (macos_window.mm), the input and closing (macos_input.mm)
    void window();
    void input(gran::platform::Window& window);
    void closing(gran::platform::Window& window);
    // Проверки по находкам ревью Т-51 (macos_review.mm) / The checks for the review T-51 findings (macos_review.mm)
    void review(gran::platform::Window& window, gran::rhi::RenderBackend& backend, Scene& scene);

#ifdef __OBJC__
    // Помощники AppKit для теста окна (macos_cocoa.mm) / AppKit helpers for the window test (macos_cocoa.mm)
    NSWindow* nativeWindow(gran::platform::Window& window);
    std::vector<gran::platform::Event> collect(gran::platform::Window& window, double seconds);
    NSEvent* mouseEvent(NSWindow* window, NSEventType type, NSPoint point, NSEventModifierFlags flags);
    NSEvent* keyEvent(NSWindow* window, NSEventType type, unsigned short code, NSString* characters,
        NSEventModifierFlags flags);
    void deliver(NSWindow* window, NSEvent* event, bool queue);
#endif

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
