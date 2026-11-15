#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"

namespace gran::platform::cocoa
{
    namespace
    {
        /*
        ** Системный курсор NSCursor по форме (порядок CursorShape — как style::Cursor). Отдельного курсора «move»
        ** у AppKit нет: берётся сжатая рука, как у перетаскивания содержимого в программах macOS.
        **
        ** The NSCursor system cursor by the shape (the CursorShape order is as style::Cursor). AppKit has no separate
        ** "move" cursor: the closed hand is used, as for dragging content in macOS programs.
        */
        NSCursor* cursorFor(CursorShape shape) noexcept
        {
            switch (shape)
            {
                case CursorShape::HAND: return [NSCursor pointingHandCursor];
                case CursorShape::TEXT: return [NSCursor IBeamCursor];
                case CursorShape::MOVE: return [NSCursor closedHandCursor];
                case CursorShape::NOT_ALLOWED: return [NSCursor operationNotAllowedCursor];
                case CursorShape::GRAB: return [NSCursor openHandCursor];
                case CursorShape::RESIZE_EW: return [NSCursor resizeLeftRightCursor];
                case CursorShape::RESIZE_NS: return [NSCursor resizeUpDownCursor];
                default: return [NSCursor arrowCursor];
            }
        }
    }

    /*
    ** Кадр вызывает setCursor после каждой отрисовки, поэтому AppKit узнаёт о курсоре только при смене формы:
    ** прямоугольники курсора вида пересчитываются (resetCursorRects), а под указателем курсор ставится сразу.
    **
    ** A frame calls setCursor after each render, so AppKit learns about the cursor only when the shape changes:
    ** the view cursor rectangles are recomputed (resetCursorRects), and under the pointer the cursor is set at once.
    */
    void CocoaWindow::setCursor(CursorShape shape)
    {
        if (shape == _shape)
            return;
        _shape = shape;
        @autoreleasepool
        {
            [_window invalidateCursorRectsForView:_view];
            NSPoint inside = [_view convertPoint:_window.mouseLocationOutsideOfEventStream fromView:nil];
            if (_window.keyWindow && NSPointInRect(inside, _view.bounds))
                [cursorFor(_shape) set];
        }
    }

    NSCursor* CocoaWindow::cursor() const noexcept
    {
        return cursorFor(_shape);
    }
}
