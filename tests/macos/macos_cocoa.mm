#include "macos_tests.hpp"

#include <chrono>

namespace macos_tests
{
    // Окно AppKit по слою из nativeHandle: тест видит только platform::Window / The AppKit window by the nativeHandle layer: the test sees only platform::Window
    NSWindow* nativeWindow(gran::platform::Window& window)
    {
        CALayer* layer = (__bridge CALayer*)window.nativeHandle();

        for (NSWindow* candidate in NSApp.windows)
        {
            if (candidate.contentView.layer == layer)
                return candidate;
        }
        throw std::runtime_error("Cocoa: no NSWindow for the CAMetalLayer");
    }

    // События окна за seconds секунд: цикл движка сам раздаёт очередь AppKit (pollEvent / waitEvents)
    // The window events within seconds: the engine loop hands out the AppKit queue itself (pollEvent / waitEvents)
    std::vector<gran::platform::Event> collect(gran::platform::Window& window, double seconds)
    {
        std::vector<gran::platform::Event> events;
        gran::platform::Event event;
        auto end = std::chrono::steady_clock::now() + std::chrono::duration<double>(seconds);

        while (std::chrono::steady_clock::now() < end)
        {
            window.waitEvents(0.02);
            while (window.pollEvent(event))
                events.push_back(event);
        }
        return events;
    }

    // Событие мыши в точке вида (пункты от левого верхнего угла) / A mouse event at a view point (points from the top left corner)
    NSEvent* mouseEvent(NSWindow* window, NSEventType type, NSPoint point, NSEventModifierFlags flags)
    {
        NSPoint location = [window.contentView convertPoint:point toView:nil];

        return [NSEvent mouseEventWithType:type location:location modifierFlags:flags
            timestamp:[NSProcessInfo processInfo].systemUptime windowNumber:window.windowNumber context:nil
            eventNumber:0 clickCount:1 pressure:1.0f];
    }

    // Клавиша по физическому коду: characters — с модификаторами, ignoring — без / A key by the physical code: characters — with modifiers, ignoring — without
    NSEvent* keyEvent(NSWindow* window, NSEventType type, unsigned short code, NSString* characters,
        NSEventModifierFlags flags)
    {
        return [NSEvent keyEventWithType:type location:NSZeroPoint modifierFlags:flags
            timestamp:[NSProcessInfo processInfo].systemUptime windowNumber:window.windowNumber context:nil
            characters:characters charactersIgnoringModifiers:characters.lowercaseString isARepeat:NO keyCode:code];
    }

    /*
    ** Доставка через очередь AppKit ([NSApp postEvent:]) — весь путь, как у настоящего ввода: цикл движка →
    ** [NSApp sendEvent:] → sendEvent: окна → вид. Клавиши AppKit отдаёт только ключевому окну: если программа на
    ** раннере не стала активной, клавиша идёт прямо в sendEvent: окна (путь от окна до вида тот же).
    **
    ** Delivering through the AppKit queue ([NSApp postEvent:]) — the whole path as for real input: the engine loop →
    ** [NSApp sendEvent:] → the window sendEvent: → the view. AppKit gives keys only to the key window: if the program did
    ** not become active on the runner, the key goes straight to the window sendEvent: (the path from the window to the view is the same).
    */
    void deliver(NSWindow* window, NSEvent* event, bool queue)
    {
        require(event != nil, "NSEvent creation");
        if (queue)
            [NSApp postEvent:event atStart:NO];
        else
            [window sendEvent:event];
    }
}
