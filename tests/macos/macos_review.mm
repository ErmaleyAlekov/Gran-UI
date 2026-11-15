#include "macos_tests.hpp"
#include "gran/rhi/metal/metal_backend.hpp"

#include <chrono>
#include <iostream>

namespace macos_tests
{
    namespace
    {
        using gran::platform::Event;
        using gran::platform::EventKind;

        // Control+перетаскивание левой — движение правой кнопкой, как нажатие и отпускание (ревью Т-51)
        // Control+left drag is a right button move, as the press and the release (review T-51)
        void controlDrag(gran::platform::Window& window, NSWindow* native)
        {
            bool right = false;

            deliver(native, mouseEvent(native, NSEventTypeLeftMouseDown, NSMakePoint(14, 14), NSEventModifierFlagControl), true);
            deliver(native, mouseEvent(native, NSEventTypeLeftMouseDragged, NSMakePoint(18, 18), NSEventModifierFlagControl), true);
            deliver(native, mouseEvent(native, NSEventTypeLeftMouseUp, NSMakePoint(18, 18), NSEventModifierFlagControl), true);
            for (const Event& event : collect(window, 0.4))
                right = right || (event.kind == EventKind::MOUSE_MOVE && event.button == gran::platform::MouseButton::RIGHT);
            require(right, "Cocoa mouse: Control+drag moves with the right button");
        }

        /*
        ** Свёрнутое окно: платформа по уведомлению ОС отмечает слой, кадр пропускается сразу (OCCLUDED), а не через
        ** секунду ожидания drawable (ревью Т-51). Если ОС на раннере не прислала уведомление, проверка только сообщает.
        **
        ** A minimized window: the platform marks the layer by the OS notification, the frame is skipped at once (OCCLUDED)
        ** instead of after a second of waiting for a drawable (review T-51). If the OS on the runner sent no notification,
        ** the check only reports.
        */
        void occluded(gran::platform::Window& window, gran::rhi::RenderBackend& backend, Scene& scene)
        {
            NSWindow* native = nativeWindow(window);
            NSString* key = @(gran::rhi::METAL_LAYER_OCCLUDED_KEY);

            [native miniaturize:nil];
            for (unsigned step = 0; step < 15 && ![[native.contentView.layer valueForKey:key] boolValue]; ++step)
                collect(window, 0.2);
            bool marked = [[native.contentView.layer valueForKey:key] boolValue];
            auto start = std::chrono::steady_clock::now();
            gran::rhi::FrameStatus status = backend.render(scene.frame);
            auto spent = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
            std::cout << "Cocoa minimized: occlusion mark " << marked << ", status " << int(status) << ", "
                << spent.count() << " ms\n";
            if (marked)
                require(status == gran::rhi::FrameStatus::OCCLUDED && spent.count() < 300, "Cocoa minimized frame skipped at once");
            [native deminiaturize:nil];
            collect(window, 1.0);
        }

        // Мёртвая клавиша с Option (characters пуст) — набор текста: KEY_DOWN без ALT (ревью Т-51)
        // A dead key with Option (characters is empty) is typing: KEY_DOWN without ALT (review T-51)
        void deadKey(gran::platform::Window& window, NSWindow* native)
        {
            bool typed = false;

            deliver(native, keyEvent(native, NSEventTypeKeyDown, 0x0E, @"", NSEventModifierFlagOption), native.keyWindow);
            for (const Event& event : collect(window, 0.4))
                typed = typed || (event.kind == EventKind::KEY_DOWN && event.key == gran::platform::Key::E && event.modifiers == 0);
            require(typed, "Cocoa keyboard: Option dead key → KEY_DOWN without ALT");
        }
    }

    // Проверки по находкам ревью Т-51; мёртвая клавиша — последней: метод ввода может оставить набираемую строку
    // The checks for the review T-51 findings; the dead key goes last: the input method may leave a composed string
    void review(gran::platform::Window& window, gran::rhi::RenderBackend& backend, Scene& scene)
    {
        NSWindow* native = nativeWindow(window);

        controlDrag(window, native);
        occluded(window, backend, scene);
        deadKey(window, native);
    }
}
