#include "gran/platform/x11/x11_window.hpp"
#include <X11/cursorfont.h>

namespace gran::platform::x11
{
    /*
    ** Курсор интерфейса запоминается; окну он задаётся через applyCursor. Кадр вызывает setCursor после
    ** каждой отрисовки, поэтому сервер получает новый курсор только при смене формы.
    **
    ** The interface cursor is remembered; applyCursor sets it on the window. A frame calls setCursor after
    ** each render, so the server receives a new cursor only when the shape changes.
    */
    void X11Window::setCursor(CursorShape shape)
    {
        _shape = shape;
        applyCursor();
    }

    /*
    ** Край своей рамки под точкой (пиксели окна) — направление EWMH _NET_WM_MOVERESIZE (0 — верхний левый угол,
    ** далее по часовой стрелке до 7 — левый край) или -1. Состояние «развёрнуто» спрашивается у WM только
    ** на краю: это запрос к серверу, а движение указателя частое.
    **
    ** The custom frame edge under the point (window pixels) — an EWMH _NET_WM_MOVERESIZE direction (0 — the top left
    ** corner, then clockwise up to 7 — the left edge) or -1. The maximized state is asked from the WM only
    ** on an edge: it is a server request, and pointer motion is frequent.
    */
    long X11Window::frameEdge(int x, int y) const
    {
        if (!_custom || !_resizable)
            return -1;
        int edge = static_cast<int>(6 * _scale);
        bool left = x < edge; bool right = x >= _pixels.width - edge;
        bool top = y < edge; bool bottom = y >= _pixels.height - edge;
        if (!(left || right || top || bottom) || isMaximized())
            return -1;
        long direction = 0;
        if (top) { if (right) direction = 2; else if (!left) direction = 1; }
        else if (bottom) { direction = 6; if (right) direction = 4; else if (!left) direction = 5; }
        else { direction = 7; if (right) direction = 3; }
        return direction;
    }

    /*
    ** Край отслеживается только без нажатых кнопок: при перетаскивании ползунка или выделении у края
    ** курсор интерфейса не должен меняться на стрелку растягивания.
    **
    ** The edge is tracked only with no buttons pressed: while dragging a slider or selecting near the edge
    ** the interface cursor must not turn into a resize arrow.
    */
    void X11Window::trackFrameCursor(const XMotionEvent& event)
    {
        if ((event.state & (Button1Mask | Button2Mask | Button3Mask)) != 0)
            return;
        long edge = frameEdge(event.x, event.y);
        if (edge == _edge)
            return;
        _edge = edge;
        applyCursor();
    }

    // Форма из cursorfont: край рамки важнее курсора интерфейса; та же форма — без запросов к серверу.
    // The cursorfont shape: the frame edge wins over the interface cursor; the same shape — no server requests.
    void X11Window::applyCursor()
    {
        static const unsigned SHAPES[] = {XC_left_ptr, XC_hand2, XC_xterm, XC_fleur, XC_X_cursor, XC_hand1, XC_sb_h_double_arrow, XC_sb_v_double_arrow};
        static const unsigned EDGES[] = {XC_top_left_corner, XC_top_side, XC_top_right_corner, XC_right_side,
            XC_bottom_right_corner, XC_bottom_side, XC_bottom_left_corner, XC_left_side};
        unsigned index = static_cast<unsigned>(_shape);
        if (index >= sizeof(SHAPES) / sizeof(SHAPES[0])) index = 0;
        unsigned shape = SHAPES[index];
        if (_edge >= 0 && _edge < 8) shape = EDGES[_edge];
        if (_native.cursor != 0 && shape == _fontShape)
            return;
        ::Cursor cursor = XCreateFontCursor(_native.display, shape);
        XDefineCursor(_native.display, _native.window, cursor);
        if (_native.cursor != 0) XFreeCursor(_native.display, _native.cursor);
        _native.cursor = cursor;
        _fontShape = shape;
    }
}
