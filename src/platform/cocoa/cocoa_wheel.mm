#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"

namespace gran::platform::cocoa
{
    /*
    ** Колесо и трекпад. Точные источники (трекпад, Magic Mouse: hasPreciseScrollingDeltas) дают пункты — они
    ** делятся на WHEEL_NOTCH_PIXELS; обычное колесо даёт строки, которые и есть «щелчки». Знак scrollingDelta
    ** уже учитывает «естественную» прокрутку и совпадает с контрактом: плюс по y — к началу документа, плюс по x —
    ** к началу строки. Инерцию после жеста ОС присылает такими же событиями (momentumPhase).
    **
    ** The wheel and the trackpad. Precise sources (a trackpad, Magic Mouse: hasPreciseScrollingDeltas) give points — they
    ** are divided by WHEEL_NOTCH_PIXELS; a regular wheel gives lines, which are the "notches". The scrollingDelta sign
    ** already accounts for "natural" scrolling and matches the contract: plus y — to the document start, plus x — to the
    ** line start. The OS sends the inertia after a gesture as the same events (momentumPhase).
    */
    void CocoaWindow::wheel(NSEvent* native, Vec2 position) noexcept
    {
        Event event;
        double x = native.scrollingDeltaX;
        double y = native.scrollingDeltaY;

        if (native.hasPreciseScrollingDeltas)
        {
            x /= WHEEL_NOTCH_PIXELS;
            y /= WHEEL_NOTCH_PIXELS;
        }
        // События одной смены фазы жеста (начало, конец) приходят без сдвига / The gesture phase change events (begin, end) come without a delta
        if (x == 0 && y == 0)
            return;
        event.kind = EventKind::MOUSE_WHEEL;
        event.position = position;
        event.modifiers = modifiersFromFlags(native.modifierFlags);
        event.wheel = {static_cast<float>(x), static_cast<float>(y)};
        post(event);
    }
}
