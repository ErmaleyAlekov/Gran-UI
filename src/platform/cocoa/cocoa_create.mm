#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran::platform::cocoa
{
    namespace
    {
        /*
        ** Стиль окна: заголовок и кнопки ОС есть всегда (своя рамка — это содержимое под прозрачным заголовком,
        ** FullSizeContentView, а «светофор» macOS остаётся). Диалог и инструмент не сворачиваются в Dock.
        **
        ** The window style: the OS title and buttons are always there (a custom frame is the content under a transparent
        ** title, FullSizeContentView, and the macOS "traffic lights" stay). A dialog and a tool do not minimize to the Dock.
        */
        NSWindowStyleMask styleMask(const WindowDesc& desc) noexcept
        {
            NSWindowStyleMask mask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;

            if (desc.kind == WindowKind::NORMAL)
                mask |= NSWindowStyleMaskMiniaturizable;
            if (desc.resizable)
                mask |= NSWindowStyleMaskResizable;
            if (desc.titleBar == TitleBar::CUSTOM)
                mask |= NSWindowStyleMaskFullSizeContentView;
            return mask;
        }
    }

    // Окно, вид с CAMetalLayer и делегат; размер — в пунктах (логических пикселях), как у WindowDesc
    // The window, the view with CAMetalLayer and the delegate; the size is in points (logical pixels), as in WindowDesc
    void CocoaWindow::create(const WindowDesc& desc)
    {
        NSRect rect = NSMakeRect(0, 0, desc.width, desc.height);

        _window = [[GranCocoaNativeWindow alloc] initWithContentRect:rect styleMask:styleMask(desc)
            backing:NSBackingStoreBuffered defer:NO];
        if (_window == nil)
            throw GranException(ErrorCode::PLATFORM, i18n::format(i18n::Message::PLATFORM_WINDOW_FAILED, {"NSWindow"}));
        // Под ARC окно не должно освобождать себя само при close / Under ARC the window must not release itself on close
        _window.releasedWhenClosed = NO;
        _window.owner = this;
        _view = [[GranCocoaView alloc] initWithOwner:this frame:rect];
        _window.contentView = _view;
        if (![_view.layer isKindOfClass:[CAMetalLayer class]])
            throw GranException(ErrorCode::PLATFORM, i18n::format(i18n::Message::PLATFORM_WINDOW_FAILED, {"CAMetalLayer"}));
        // Слой уже проверен: id приводится к CAMetalLayer* неявно / The layer is already checked: id converts to CAMetalLayer* implicitly
        id layer = _view.layer;
        _layer = layer;
        _layer.contentsScale = _window.backingScaleFactor;
        _delegate = [[GranCocoaWindowDelegate alloc] init];
        _delegate.owner = this;
        _window.delegate = _delegate;
        [_window makeFirstResponder:_view];
    }

    /*
    ** Заголовок, наименьший размер, фон до первого кадра (sRGB, как у кадра), тёмная тема заголовка и своя рамка.
    ** Вкладки окон macOS выключены: движок сам решает, где вкладки (иначе AppKit добавит пункты меню и склейку окон).
    **
    ** The title, the minimum size, the background before the first frame (sRGB, as the frame), the dark title theme and
    ** the custom frame. macOS window tabs are off: the engine decides about tabs itself (otherwise AppKit adds menu
    ** items and merges windows).
    */
    void CocoaWindow::configure(const WindowDesc& desc)
    {
        NSString* title = [NSString stringWithUTF8String:desc.title.c_str()];

        if (title != nil)
            _window.title = title;
        _window.contentMinSize = NSMakeSize(desc.minWidth, desc.minHeight);
        _window.backgroundColor = [NSColor colorWithSRGBRed:desc.background.r green:desc.background.g
            blue:desc.background.b alpha:1.0];
        _window.tabbingMode = NSWindowTabbingModeDisallowed;
        if (desc.darkTitleBar)
            _window.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
        _custom = desc.titleBar == TitleBar::CUSTOM;
        if (!_custom)
            return;
        _window.titlebarAppearsTransparent = YES;
        _window.titleVisibility = NSWindowTitleHidden;
    }

    /*
    ** Место: по центру экрана или над владельцем. Владелец запоминается слабой ссылкой, дочерним окно станет
    ** в show(): так подчинённое окно держится над владельцем и двигается вместе с ним.
    **
    ** The place: in the screen center or over the owner. The owner is remembered by a weak reference, the window
    ** becomes its child in show(): so the owned window stays above the owner and moves together with it.
    */
    void CocoaWindow::place(const WindowDesc& desc)
    {
        auto* owner = dynamic_cast<CocoaWindow*>(desc.owner);

        if (owner != nullptr)
            _parent = owner->_window;
        if (desc.position != WindowPosition::CENTER_OWNER || owner == nullptr)
        {
            [_window center];
            return;
        }
        NSRect outer = owner->_window.frame;
        NSRect frame = _window.frame;
        frame.origin.x = NSMidX(outer) - frame.size.width / 2;
        frame.origin.y = NSMidY(outer) - frame.size.height / 2;
        [_window setFrame:frame display:NO];
    }
}
