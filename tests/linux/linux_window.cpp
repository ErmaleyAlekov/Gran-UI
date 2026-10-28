#include "linux_tests.hpp"
#include <chrono>
#include <thread>

namespace linux_tests
{
    namespace
    {
        // post будит poll через pipe, не вызывая Xlib из фонового потока.
        // A post wakes poll through a pipe without calling Xlib from a background thread.
        void wakeLater(gran::platform::Window* window)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(60));
            window->wake();
        }

        // Ожидание должно завершиться до общего таймаута, когда фон отправляет wake.
        // Waiting must end before its overall timeout when a background thread sends wake.
        void checkWake(gran::platform::Window& window)
        {
            gran::platform::Event event;
            while (window.pollEvent(event)) {}
            std::thread sender(wakeLater, &window);
            auto start = std::chrono::steady_clock::now();
            window.waitEvents(1);
            sender.join();
            require(std::chrono::steady_clock::now() - start < std::chrono::milliseconds(900), "wake timeout");
        }
    }

    // Проверка настоящей surface, трёх кадров, clipboard, wake и штатного close.
    // Test a real surface, three frames, clipboard, wake and normal close.
    void window()
    {
        gran::platform::WindowDesc desc; desc.width = 96; desc.height = 72; desc.title = "Gran Linux smoke";
        auto window = gran::platform::createWindow(desc);
        gran::rhi::BackendOptions options;
        options.nativeWindow = window->nativeHandle(); options.nativeDisplay = window->nativeDisplay();
        options.width = static_cast<unsigned>(window->pixelSize().width); options.height = static_cast<unsigned>(window->pixelSize().height);
        options.forceWarp = true; options.debugLayer = true;
        auto renderer = gran::rhi::createPlatformBackend(options);
        Scene scene(*renderer);
        renderer->render(scene.frame); window->show();
        for (unsigned index = 0; index < 3; ++index) renderer->render(scene.frame);
        window->setClipboardText("Берёста: UTF-8");
        clipboard(*window);
        input(*window);
        gran::platform::Event event;
        checkWake(*window);
        require(window->clipboardText() == "Берёста: UTF-8", "clipboard UTF-8");
        window->close(); bool closed = false;
        while (window->pollEvent(event)) closed |= event.kind == gran::platform::EventKind::CLOSE;
        require(closed, "close event");
        auto diagnostics = renderer->takeDiagnostics();
        if (!diagnostics.empty()) throw std::runtime_error(diagnostics.front());
    }
}
