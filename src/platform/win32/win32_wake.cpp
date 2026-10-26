#include "gran/platform/win32/win32_window.hpp"

namespace gran::platform::win32
{
    namespace
    {
        // Сообщение-будильник: оконная процедура его не разбирает (DefWindowProc), важен сам факт прихода
        // The wake-up message: the window procedure does not handle it (DefWindowProc), only its arrival matters
        constexpr UINT WAKE_MESSAGE = WM_APP + 1;
    }

    /*
    ** PostMessageW потокобезопасен и не ждёт получателя: сообщение будит
    ** MsgWaitForMultipleObjectsEx в waitEvents (QS_ALLINPUT включает QS_POSTMESSAGE),
    ** а pollEvent просто выбирает его из очереди. Переполнение очереди (10 000 сообщений)
    ** не страшно: поток интерфейса и так уже не спит, раз очередь полна.
    **
    ** PostMessageW is thread-safe and does not wait for the receiver: the message wakes
    ** MsgWaitForMultipleObjectsEx in waitEvents (QS_ALLINPUT includes QS_POSTMESSAGE),
    ** and pollEvent simply takes it from the queue. A queue overflow (10,000 messages)
    ** does no harm: the UI thread is not sleeping anyway when the queue is full.
    */
    void Win32Window::wake() noexcept
    {
        if (window != nullptr)
            PostMessageW(window, WAKE_MESSAGE, 0, 0);
    }
}
