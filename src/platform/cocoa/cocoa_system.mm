#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"

#include <cmath>

namespace gran::platform::cocoa
{
    // Поверхность бэкенда Metal — слой вида (невладеющий указатель: слоем владеет вид)
    // The Metal backend surface is the view layer (a non-owning pointer: the view owns the layer)
    void* CocoaWindow::nativeHandle() const
    {
        return (__bridge void*)_layer;
    }

    // backingScaleFactor: 2 на Retina, 1 на обычном мониторе; «масштабированные» режимы Retina AppKit рисует
    // в кадре с масштабом 2 и сам сжимает его до экрана
    // backingScaleFactor: 2 on Retina, 1 on a regular monitor; the "scaled" Retina modes are drawn by AppKit into a
    // frame with scale 2, and it shrinks the frame to the screen itself
    float CocoaWindow::scale() const
    {
        return static_cast<float>(_window.backingScaleFactor);
    }

    // Размер вида в пикселях устройства: перевод пунктов в пиксели делает AppKit (convertRectToBacking)
    // The view size in device pixels: AppKit turns points into pixels (convertRectToBacking)
    Size CocoaWindow::pixelSize() const
    {
        NSRect backing = [_view convertRectToBacking:_view.bounds];

        return {static_cast<float>(std::lround(backing.size.width)), static_cast<float>(std::lround(backing.size.height))};
    }

    /*
    ** Показ после первого кадра: дочерним владельцу окно становится только сейчас (addChildWindow: выводит его
    ** на экран), программа выходит на передний план — запущенная из терминала, она иначе осталась бы за ним.
    **
    ** Showing after the first frame: the window becomes the owner's child only now (addChildWindow: puts it on
    ** screen), the program comes to the front — run from a terminal, it would otherwise stay behind it.
    */
    void CocoaWindow::show()
    {
        @autoreleasepool
        {
            NSWindow* parent = _parent;
            if (parent != nil && _window.parentWindow == nil)
                [parent addChildWindow:_window ordered:NSWindowAbove];
            [NSApp activateIgnoringOtherApps:YES];
            [_window makeKeyAndOrderFront:nil];
            if (_startMaximized && !_window.zoomed)
                [_window zoom:nil];
            _startMaximized = false;
        }
    }

    // Окно поднимается из Dock, выходит на передний план и получает фокус / The window comes back from the Dock, comes to the front and gets the focus
    void CocoaWindow::activate()
    {
        @autoreleasepool
        {
            if (_window.miniaturized)
                [_window deminiaturize:nil];
            [NSApp activateIgnoringOtherApps:YES];
            [_window makeKeyAndOrderFront:nil];
        }
    }
}
