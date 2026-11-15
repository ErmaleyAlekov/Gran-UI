#include "macos_tests.hpp"

#import <Cocoa/Cocoa.h>

#include <chrono>
#include <iostream>
#include <thread>

namespace macos_tests
{
    namespace
    {
        // Фоновый поток будит цикл окна: событие AppKit из чужого потока (postEvent:atStart:)
        // A background thread wakes the window loop: an AppKit event from a foreign thread (postEvent:atStart:)
        void wakeLater(gran::platform::Window* window)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(60));
            window->wake();
        }

        // Ожидание с пределом в секунду кончается раньше: его прервал wake() / Waiting with a one second limit ends earlier: wake() interrupted it
        void checkWake(gran::platform::Window& window)
        {
            gran::platform::Event event;

            while (window.pollEvent(event)) {}
            std::thread sender(wakeLater, &window);
            auto start = std::chrono::steady_clock::now();
            window.waitEvents(1);
            sender.join();
            require(std::chrono::steady_clock::now() - start < std::chrono::milliseconds(900), "Cocoa wake timeout");
        }

        // Новый размер окна → RESIZE в пикселях устройства, кадр нового размера читается своим чтением
        // A new window size → RESIZE in device pixels, the frame of the new size is read by the own readback
        void checkResize(gran::platform::Window& window, gran::rhi::RenderBackend& backend, Scene& scene)
        {
            NSWindow* native = nativeWindow(window);
            gran::platform::Event event;
            bool resized = false;

            [native setContentSize:NSMakeSize(120, 90)];
            while (window.pollEvent(event))
                resized = resized || event.kind == gran::platform::EventKind::RESIZE;
            gran::Size pixels = window.pixelSize();
            require(resized && pixels.width == 120 * window.scale(), "Cocoa RESIZE in device pixels");
            backend.resize(static_cast<unsigned>(pixels.width), static_cast<unsigned>(pixels.height));
            backend.render(scene.frame);
            compare(backend, 8, 8, 0xff0000ff);
            compare(backend, static_cast<unsigned>(pixels.width) - 4, 4, 0xffff0000);
        }
    }

    /*
    ** Настоящее окно Cocoa на раннере (у него есть сеанс рабочего стола и устройство Metal): слой CAMetalLayer,
    ** кадры через nextDrawable, чтение кадра бэкендом (без прав «Запись экрана»), ввод через [NSApp postEvent:]
    ** (без прав «Универсальный доступ»), смена размера, буфер обмена, wake из потока и закрытие событием CLOSE.
    **
    ** A real Cocoa window on the runner (it has a desktop session and a Metal device): the CAMetalLayer layer, frames
    ** through nextDrawable, reading the frame by the backend (without the "Screen Recording" rights), input through
    ** [NSApp postEvent:] (without the "Accessibility" rights), resizing, the clipboard, wake from a thread and closing by CLOSE.
    */
    void window()
    {
        gran::platform::WindowDesc desc;
        desc.width = 96; desc.height = 72; desc.title = "Gran macOS smoke";
        auto window = gran::platform::createWindow(desc);
        gran::rhi::BackendOptions options;
        options.nativeWindow = window->nativeHandle(); options.debugLayer = true;
        options.width = static_cast<unsigned>(window->pixelSize().width); options.height = static_cast<unsigned>(window->pixelSize().height);
        require(options.width == static_cast<unsigned>(96 * window->scale()), "pixelSize = points × backingScaleFactor");
        auto renderer = gran::rhi::createPlatformBackend(options);
        Scene scene(*renderer);
        renderer->render(scene.frame); window->show();
        for (unsigned index = 0; index < 3; ++index)
            require(renderer->render(scene.frame) == gran::rhi::FrameStatus::PRESENTED, "Cocoa frame presented");
        compare(*renderer, 8, 8, 0xff0000ff); compare(*renderer, 40, 30, 0xffff0000);
        std::cout << "Cocoa window: scale " << window->scale() << ", key " << int(nativeWindow(*window).keyWindow) << '\n';
        input(*window);
        checkResize(*window, *renderer, scene);
        review(*window, *renderer, scene);
        window->setClipboardText("Берёста: UTF-8");
        require(window->clipboardText() == "Берёста: UTF-8", "Cocoa clipboard UTF-8");
        checkWake(*window);
        closing(*window);
        auto diagnostics = renderer->takeDiagnostics();
        if (!diagnostics.empty()) throw std::runtime_error(diagnostics.front());
    }
}
