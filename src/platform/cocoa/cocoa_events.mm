#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"

#include <atomic>

namespace gran::platform::cocoa
{
    namespace
    {
        // Событие пробуждения уже в очереди AppKit: повторные wake() его не множат
        // The wake event is already in the AppKit queue: repeated wake() calls do not multiply it
        std::atomic<bool>& wakePosted() noexcept
        {
            static std::atomic<bool> posted{false};

            return posted;
        }

        /*
        ** Одно событие AppKit: ждать его не дольше until, раздать окнам ([NSApp sendEvent:] → sendEvent: окна →
        ** вид). Своё событие пробуждения не раздаётся, только снимает флаг. Пул автоосвобождения — на каждое
        ** событие: [NSApp run] не вызывается, и без своего пула объекты AppKit копились бы до выхода.
        **
        ** One AppKit event: wait for it at most until, hand it to the windows ([NSApp sendEvent:] → the window sendEvent: →
        ** the view). The own wake event is not handed out, it only clears the flag. An autorelease pool per event:
        ** [NSApp run] is not called, and without an own pool the AppKit objects would pile up until the exit.
        */
        bool pumpOne(NSDate* until)
        {
            @autoreleasepool
            {
                NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:until
                    inMode:NSDefaultRunLoopMode dequeue:YES];
                if (event == nil)
                    return false;
                if (event.type == NSEventTypeApplicationDefined && event.subtype == WAKE_SUBTYPE)
                {
                    wakePosted() = false;
                    return true;
                }
                [NSApp sendEvent:event];
                return true;
            }
        }
    }

    // Первое событие ждём до until, остальные накопившиеся раздаём без ожидания
    // The first event is awaited until until, the rest that piled up are handed out without waiting
    void CocoaWindow::pump(NSDate* until)
    {
        while (pumpOne(until))
            until = [NSDate distantPast];
    }

    // Очередь пуста — сначала раздать то, что AppKit уже получил, без ожидания
    // The queue is empty — first hand out what AppKit has already received, without waiting
    bool CocoaWindow::pollEvent(Event& event)
    {
        if (_events.empty())
            pump([NSDate distantPast]);
        if (_events.empty())
            return false;
        event = _events.front();
        _events.pop_front();
        return true;
    }

    // Спать нельзя, пока у любого окна есть события: цикл приложения ждёт только на главном окне
    // Sleeping is not allowed while any window has events: the application loop waits on the main window only
    void CocoaWindow::waitEvents(double timeout)
    {
        NSDate* until = [NSDate distantFuture];

        for (CocoaWindow* window : registry())
        {
            if (!window->_events.empty())
                return;
        }
        if (timeout >= 0)
            until = [NSDate dateWithTimeIntervalSinceNow:timeout];
        pump(until);
    }

    /*
    ** Из любого потока: событие NSEventTypeApplicationDefined в очередь AppKit (postEvent:atStart: AppKit разрешает
    ** вызывать из других потоков). Пул — свой: у чужого потока его может не быть.
    **
    ** From any thread: an NSEventTypeApplicationDefined event into the AppKit queue (AppKit allows calling
    ** postEvent:atStart: from other threads). The pool is own: a foreign thread may have none.
    */
    void CocoaWindow::wake() noexcept
    {
        if (wakePosted().exchange(true))
            return;
        @autoreleasepool
        {
            NSEvent* event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined location:NSZeroPoint
                modifierFlags:0 timestamp:0 windowNumber:0 context:nil subtype:WAKE_SUBTYPE data1:0 data2:0];
            [NSApp postEvent:event atStart:NO];
        }
    }
}
