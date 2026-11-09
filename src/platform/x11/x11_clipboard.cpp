#include "gran/platform/x11/x11_window.hpp"
#include <X11/Xatom.h>
#include <chrono>
#include <poll.h>

namespace gran::platform::x11
{
    // Цель STRING по ICCCM — Latin-1: отдаём её только для ASCII, иначе кириллица придёт мусором.
    // The STRING target is Latin-1 by ICCCM: serve it only for ASCII, otherwise Cyrillic arrives as garbage.
    static bool asciiOnly(const std::string& text)
    {
        for (unsigned char byte : text)
        {
            if (byte >= 0x80)
                return false;
        }
        return true;
    }

    // CLIPBOARD принадлежит окну до завершения приложения или передачи другому владельцу.
    // The window owns CLIPBOARD until application exit or transfer to another owner.
    void X11Window::setClipboardText(const std::string& text)
    {
        _clipboard = text;
        XSetSelectionOwner(_native.display, atom("CLIPBOARD"), _native.window, CurrentTime);
        XFlush(_native.display);
    }

    // Ожидание ограничено; чужое приложение не может навсегда заблокировать UI при вставке.
    // Waiting is bounded; another application cannot block the UI forever during paste.
    std::string X11Window::clipboardText()
    {
        if (XGetSelectionOwner(_native.display, atom("CLIPBOARD")) == _native.window) return _clipboard;
        _clipboardReady = false; _clipboard.clear();
        XConvertSelection(_native.display, atom("CLIPBOARD"), atom("UTF8_STRING"), atom("GRAN_CLIPBOARD"), _native.window, CurrentTime);
        XFlush(_native.display);
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
        while (!_clipboardReady && std::chrono::steady_clock::now() < deadline)
        {
            if (XPending(_native.display) == 0)
            {
                pollfd descriptor{ConnectionNumber(_native.display), POLLIN, 0};
                ::poll(&descriptor, 1, 10);
            }
            while (XPending(_native.display) != 0)
            { XEvent event{}; XNextEvent(_native.display, &event); if (!XFilterEvent(&event, _native.window)) dispatch(event); }
        }
        return _clipboard;
    }

    // TARGETS сообщает UTF-8 и STRING; размер ответа ограничен безопасным размером X-запроса.
    // TARGETS advertises UTF-8 and STRING; reply size is limited to a safe X request size.
    void X11Window::selectionRequest(const XSelectionRequestEvent& request)
    {
        XEvent reply{};
        reply.xselection.type = SelectionNotify; reply.xselection.display = request.display; reply.xselection.requestor = request.requestor;
        reply.xselection.selection = request.selection; reply.xselection.target = request.target; reply.xselection.time = request.time;
        Atom property = request.property;
        if (property == None) property = request.target;
        if (request.target == atom("TARGETS"))
        {
            Atom targets[] = {atom("TARGETS"), atom("UTF8_STRING"), XA_STRING};
            XChangeProperty(_native.display, request.requestor, property, XA_ATOM, 32, PropModeReplace,
                reinterpret_cast<unsigned char*>(targets), 3); reply.xselection.property = property;
        }
        else if ((request.target == atom("UTF8_STRING") || (request.target == XA_STRING && asciiOnly(_clipboard)))
            && _clipboard.size() <= 200000)
        {
            XChangeProperty(_native.display, request.requestor, property, request.target, 8, PropModeReplace,
                reinterpret_cast<const unsigned char*>(_clipboard.data()), static_cast<int>(_clipboard.size()));
            reply.xselection.property = property;
        }
        XSendEvent(_native.display, request.requestor, False, 0, &reply); XFlush(_native.display);
    }

    // Тип и размер свойства проверяются до чтения; INCR пока не принимается.
    // Check property type and size before reading; INCR is not accepted yet.
    void X11Window::selection(XEvent& event)
    {
        if (event.type == SelectionRequest) { selectionRequest(event.xselectionrequest); return; }
        if (event.type == SelectionClear) return;
        // Данные броска XDND, а не буфера обмена / The XDND drop data, not the clipboard
        if (event.xselection.selection == atom("XdndSelection")) { dndData(event.xselection); return; }
        _clipboardReady = true;
        if (event.xselection.property == None) return;
        Atom type = 0; int format = 0; unsigned long count = 0; unsigned long remaining = 0;
        unsigned char* bytes = nullptr;
        int result = XGetWindowProperty(_native.display, _native.window, event.xselection.property, 0, 4194304,
            True, AnyPropertyType, &type, &format, &count, &remaining, &bytes);
        if (result == Success && type == atom("UTF8_STRING") && format == 8 && remaining == 0 && bytes != nullptr)
            _clipboard.assign(reinterpret_cast<char*>(bytes), count);
        if (bytes != nullptr) XFree(bytes);
    }
}
