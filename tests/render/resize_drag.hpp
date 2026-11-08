#ifndef GRAN_TESTS_RESIZE_DRAG_HPP
#define GRAN_TESTS_RESIZE_DRAG_HPP

#include "resize_capture.hpp"

#include <atomic>
#include <memory>
#include <thread>

namespace render_tests
{
    // Перетаскивание правого нижнего угла: откуда, сколько шагов, на сколько пикселей, через сколько мс
    // и сколько кругов «вправо и обратно»
    // Dragging the bottom right corner: from where, how many steps, by how many pixels, how many ms apart
    // and how many "right and back" rounds
    struct DragPlan
    {
        POINT corner{};
        int steps = 0;
        int step = 0;
        DWORD gapMs = 0;
        int rounds = 1;
    };

    // Общее для потока «мыши» и потока окна; done ставит поток мыши, когда кнопка отпущена
    // Shared by the "mouse" thread and the window thread; the mouse thread sets done when the button is released
    struct DragState
    {
        DragPlan plan;
        std::atomic<bool> done{false};
        std::thread thread;
    };

    /*
    ** Точка правого нижнего угла рамки, за которую Windows начинает изменение размера (WM_NCHITTEST
    ** отвечает HTBOTTOMRIGHT); false — такой точки нет, и тянуть мышью нельзя.
    **
    ** The point of the bottom right frame corner where Windows starts resizing (WM_NCHITTEST answers
    ** HTBOTTOMRIGHT); false — there is no such point, and the mouse cannot drag.
    */
    bool resizeCorner(HWND window, POINT& corner);

    /*
    ** Настоящее перетаскивание мышью (SendInput) в отдельном потоке: поток окна в это время крутит
    ** модальный цикл Windows, как при работе человека. Курсор пользователя двигается — только на стенде.
    **
    ** A real mouse drag (SendInput) in a separate thread: the window thread meanwhile runs the Windows modal
    ** loop, as when a person works. The user's cursor moves — only on the stand.
    */
    void startDrag(const std::shared_ptr<DragState>& state);
}

#endif
