#include "macos_tests.hpp"

#include <cmath>
#include <iostream>

namespace macos_tests
{
    namespace
    {
        using gran::platform::Event;
        using gran::platform::EventKind;
        using gran::platform::MouseButton;

        /*
        ** Мышь через очередь AppKit: нажатие, перетаскивание и отпускание левой (координаты — пункты вида), правая
        ** кнопка и Control+щелчок (соглашение macOS — тоже правая, без META).
        **
        ** The mouse through the AppKit queue: a left press, drag and release (the coordinates are view points), the right
        ** button and Control+click (the macOS convention — right too, without META).
        */
        void mouseChecks(gran::platform::Window& window, NSWindow* native)
        {
            bool down = false; bool moved = false; bool up = false; unsigned right = 0;

            deliver(native, mouseEvent(native, NSEventTypeLeftMouseDown, NSMakePoint(20, 30), 0), true);
            deliver(native, mouseEvent(native, NSEventTypeLeftMouseDragged, NSMakePoint(25, 35), 0), true);
            deliver(native, mouseEvent(native, NSEventTypeLeftMouseUp, NSMakePoint(25, 35), 0), true);
            deliver(native, mouseEvent(native, NSEventTypeRightMouseDown, NSMakePoint(10, 10), 0), true);
            deliver(native, mouseEvent(native, NSEventTypeRightMouseUp, NSMakePoint(10, 10), 0), true);
            deliver(native, mouseEvent(native, NSEventTypeLeftMouseDown, NSMakePoint(12, 12), NSEventModifierFlagControl), true);
            deliver(native, mouseEvent(native, NSEventTypeLeftMouseUp, NSMakePoint(12, 12), NSEventModifierFlagControl), true);
            for (const Event& event : collect(window, 0.5))
            {
                bool near = std::fabs(event.position.x - 20) < 0.5f && std::fabs(event.position.y - 30) < 0.5f;
                down = down || (event.kind == EventKind::MOUSE_DOWN && event.button == MouseButton::LEFT && near && event.clicks == 1);
                moved = moved || (event.kind == EventKind::MOUSE_MOVE && std::fabs(event.position.x - 25) < 0.5f);
                up = up || (event.kind == EventKind::MOUSE_UP && event.button == MouseButton::LEFT);
                if (event.kind == EventKind::MOUSE_DOWN && event.button == MouseButton::RIGHT
                    && (event.modifiers & gran::platform::MODIFIER_META) == 0)
                    ++right;
            }
            require(down && moved && up, "Cocoa mouse: left press, drag, release in view points");
            require(right == 2, "Cocoa mouse: right button and Control+click");
        }

        /*
        ** Клавиатура: Shift+A даёт KEY_DOWN и символ «A» (через метод ввода), Cmd+C — KEY_DOWN с Mod без символа,
        ** Control+Tab — Mod+Tab (контракт платформы).
        **
        ** The keyboard: Shift+A gives KEY_DOWN and the "A" symbol (through the input method), Cmd+C — KEY_DOWN with Mod
        ** without a symbol, Control+Tab — Mod+Tab (the platform contract).
        */
        void keyChecks(gran::platform::Window& window, NSWindow* native)
        {
            bool queue = native.keyWindow;
            bool letter = false; bool character = false; bool copy = false; bool tab = false; bool stray = false;

            deliver(native, keyEvent(native, NSEventTypeKeyDown, 0x00, @"A", NSEventModifierFlagShift), queue);
            deliver(native, keyEvent(native, NSEventTypeKeyUp, 0x00, @"A", NSEventModifierFlagShift), queue);
            deliver(native, keyEvent(native, NSEventTypeKeyDown, 0x08, @"c", NSEventModifierFlagCommand), queue);
            deliver(native, keyEvent(native, NSEventTypeKeyDown, 0x30, @"\t", NSEventModifierFlagControl), queue);
            for (const Event& event : collect(window, 0.5))
            {
                std::uint8_t mods = event.modifiers;
                if (event.kind == EventKind::KEY_DOWN || event.kind == EventKind::CHARACTER)
                    std::cout << "Cocoa key event: kind " << int(event.kind) << " key " << gran::platform::keyName(event.key)
                        << " mods " << int(mods) << " char " << std::uint32_t(event.character) << '\n';
                letter = letter || (event.kind == EventKind::KEY_DOWN && event.key == gran::platform::Key::A
                    && (mods & gran::platform::MODIFIER_SHIFT) != 0);
                character = character || (event.kind == EventKind::CHARACTER && event.character == U'A');
                stray = stray || (event.kind == EventKind::CHARACTER && event.character == U'c');
                copy = copy || (event.kind == EventKind::KEY_DOWN && event.key == gran::platform::Key::C
                    && (mods & gran::platform::MODIFIER_CONTROL) != 0);
                tab = tab || (event.kind == EventKind::KEY_DOWN && event.key == gran::platform::Key::TAB
                    && mods == gran::platform::MODIFIER_CONTROL);
            }
            if (queue)
                std::cout << "Cocoa keys delivered through [NSApp postEvent:]\n";
            else
                std::cout << "Cocoa keys delivered through [NSWindow sendEvent:] (the window is not key)\n";
            require(letter && character && !stray, "Cocoa keyboard: Shift+A → KEY_DOWN + CHARACTER, Cmd+C without a symbol");
            require(copy, "Cocoa keyboard: Cmd+C → Mod+C");
            require(tab, "Cocoa keyboard: Control+Tab → Mod+Tab");
        }

        /*
        ** Точная прокрутка (как у трекпада): 96 пунктов → 2 щелчка. Событие прокрутки AppKit отдаёт окну под настоящим
        ** указателем, а двигать указатель тест без прав «Универсального доступа» не может — поэтому прямо в окно.
        **
        ** Precise scrolling (as from a trackpad): 96 points → 2 notches. AppKit gives a scroll event to the window under
        ** the real pointer, and the test cannot move the pointer without the "Accessibility" rights — so straight to the window.
        */
        void wheelCheck(gran::platform::Window& window, NSWindow* native)
        {
            CGEventRef source = CGEventCreateScrollWheelEvent(nullptr, kCGScrollEventUnitPixel, 2, 96, 0);
            NSEvent* scroll = [NSEvent eventWithCGEvent:source];
            float wheel = 0;

            CFRelease(source);
            deliver(native, scroll, false);
            for (const Event& event : collect(window, 0.2))
            {
                if (event.kind == EventKind::MOUSE_WHEEL)
                    wheel += event.wheel.y;
            }
            std::cout << "Cocoa precise wheel: " << wheel << " notches\n";
            require(std::fabs(wheel - 2.0f) < 0.01f, "Cocoa wheel: 96 precise points = +2 notches");
        }
    }

    // Ввод в окно Cocoa: мышь, клавиатура, колесо / Input into the Cocoa window: the mouse, the keyboard, the wheel
    void input(gran::platform::Window& window)
    {
        NSWindow* native = nativeWindow(window);

        collect(window, 0.3);
        mouseChecks(window, native);
        keyChecks(window, native);
        wheelCheck(window, native);
    }

    // Кнопка закрытия даёт CLOSE и окно не закрывает; close() — то же событие (контракт платформы)
    // The close button gives CLOSE and does not close the window; close() is the same event (the platform contract)
    void closing(gran::platform::Window& window)
    {
        NSWindow* native = nativeWindow(window);
        bool button = false;
        bool call = false;

        [native performClose:nil];
        for (const Event& event : collect(window, 0.2))
            button = button || event.kind == EventKind::CLOSE;
        require(button && native.visible, "Cocoa close button → CLOSE, the window stays open");
        window.close();
        for (const Event& event : collect(window, 0.1))
            call = call || event.kind == EventKind::CLOSE;
        require(call, "Cocoa close() → CLOSE");
    }
}
