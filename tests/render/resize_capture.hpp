#ifndef GRAN_TESTS_RESIZE_CAPTURE_HPP
#define GRAN_TESTS_RESIZE_CAPTURE_HPP

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cstdint>

namespace render_tests
{
    /*
    ** Серая подложка (128, 128, 128) сразу под окном теста: в щели, где DWM ещё показывает старые границы
    ** окна, виден серый, а не случайный рабочий стол. Так белое и чёрное на снимке — только мусор самого окна.
    **
    ** A gray backdrop (128, 128, 128) right below the test window: in the gap where DWM still shows the old
    ** window bounds gray is seen, not a random desktop. So white and black in the capture are only the
    ** window's own garbage.
    */
    HWND createBackdrop(HWND above, const RECT& area);

    // Главное (самое большое видимое) окно приложения в текущем потоке; nullptr, пока окно не показано
    // The application main (the largest visible) window in the current thread; nullptr until the window is shown
    HWND threadWindow();

    // Число столбцов изображения BGRA (строки через pitch байт), которые больше чем наполовину чисто белые
    // или чисто чёрные: так DWM заливает часть окна, куда кадр приложения ещё не пришёл
    // The number of columns of a BGRA image (rows pitch bytes apart) that are more than half pure white
    // or pure black: this is how DWM fills the part of the window the application frame has not reached yet
    int badColumns(const std::uint8_t* pixels, unsigned pitch, int width, int height);
}

#endif
