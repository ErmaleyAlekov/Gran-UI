#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"

namespace gran::platform::cocoa
{
    // Текст общего буфера обмена (NSPasteboard) в UTF-8; не текст — пустая строка
    // The general pasteboard (NSPasteboard) text in UTF-8; not a text — an empty string
    std::string CocoaWindow::clipboardText()
    {
        @autoreleasepool
        {
            NSString* text = [[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];
            const char* bytes = text.UTF8String;
            if (bytes == nullptr)
                return std::string();
            return std::string(bytes);
        }
    }

    // Буфер очищается и получает строку; строка без верного UTF-8 буфер только очищает
    // The pasteboard is cleared and gets the string; a string without valid UTF-8 only clears the pasteboard
    void CocoaWindow::setClipboardText(const std::string& text)
    {
        @autoreleasepool
        {
            NSPasteboard* board = [NSPasteboard generalPasteboard];
            NSString* value = [[NSString alloc] initWithBytes:text.data() length:text.size()
                encoding:NSUTF8StringEncoding];
            [board clearContents];
            if (value != nil)
                [board setString:value forType:NSPasteboardTypeString];
        }
    }

    // Дерево доступности запоминается; мост к NSAccessibility — следующий шаг (дикторы пока видят только окно)
    // The accessibility tree is remembered; the bridge to NSAccessibility is a next step (screen readers see only the window for now)
    void CocoaWindow::setAccessTree(a11y::AccessTree* tree)
    {
        _access = tree;
    }

    // Без моста NSAccessibility сообщать некому / Without the NSAccessibility bridge there is nobody to notify
    void CocoaWindow::accessFocusChanged()
    {
    }

    void CocoaWindow::accessRefresh()
    {
    }
}
