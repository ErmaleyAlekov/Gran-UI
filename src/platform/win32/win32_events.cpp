#include "gran/platform/win32/win32_window.hpp"

#include <cmath>
#include <utility>

namespace gran::platform::win32
{
    // Нехватка памяти внутри оконной процедуры не должна вылететь в ОС: событие теряется
    // Running out of memory inside the window procedure must not escape into the OS: the event is lost
    void Win32Window::push(const Event& event) noexcept
    {
        try
        {
            _events.push_back(event);
        }
        catch (...)
        {
            // Событие отброшено: следующее перерисует окно целиком
            // The event is dropped: the next one repaints the window entirely
        }
    }

    // Сначала накопленные события, затем сообщения ОС без ожидания / First the accumulated events, then the OS messages without waiting
    bool Win32Window::pollEvent(Event& event)
    {
        MSG message;

        while (_events.empty() && PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        if (_events.empty())
            return false;
        event = _events.front();
        _events.pop_front();
        return true;
    }

    /*
    ** Ожидание ввода ОС без занятия процессора (MsgWaitForMultipleObjectsEx): в простое
    ** движок не тратит ни такта и не рисует кадров.
    **
    ** Waiting for OS input without occupying the processor (MsgWaitForMultipleObjectsEx): when idle
    ** the engine spends no cycles and draws no frames.
    */
    void Win32Window::waitEvents(double timeout)
    {
        DWORD milliseconds = INFINITE;

        if (!_events.empty())
            return;
        if (timeout >= 0.0)
            milliseconds = static_cast<DWORD>(std::ceil(timeout * 1000.0));
        MsgWaitForMultipleObjectsEx(0, nullptr, milliseconds, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    }

    // Клиентская область в пикселях устройства / The client area in device pixels
    Size Win32Window::pixelSize() const
    {
        RECT client{};

        GetClientRect(window, &client);
        return Size{static_cast<float>(client.right - client.left), static_cast<float>(client.bottom - client.top)};
    }

    // Обратный вызов перерисовки при изменении размера мышью / The repaint callback while resizing with the mouse
    void Win32Window::setLiveResize(std::function<void()> callback)
    {
        liveResize = std::move(callback);
    }
}
