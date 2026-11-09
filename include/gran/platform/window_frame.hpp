#ifndef GRAN_PLATFORM_WINDOW_FRAME_HPP
#define GRAN_PLATFORM_WINDOW_FRAME_HPP

#include <cstdint>

namespace gran::platform
{
    // Заголовок ОС по умолчанию; свой заголовок занимает клиентскую область.
    // The OS title bar is the default; a custom title bar occupies the client area.
    enum class TitleBar : std::uint8_t { SYSTEM, CUSTOM };

    // Роль участка разметки в управлении окном; NONE оставляет обычный ввод, RESIZE — ручка изменения размера (правый нижний угол).
    // The role of a markup region in window management; NONE keeps ordinary input, RESIZE is a size grip (the bottom-right corner).
    enum class WindowRole : std::uint8_t { NONE, DRAG, MINIMIZE, MAXIMIZE, CLOSE, RESIZE };

    /*
    ** Вид окна: обычное; диалог — без кнопок свёртки и развёртки, без значка в панели задач;
    ** инструментальное — узкий заголовок (Windows) или тип UTILITY (X11).
    **
    ** The window kind: a normal one; a dialog — without the minimize and maximize buttons, without a taskbar icon;
    ** a tool one — a narrow title bar (Windows) or the UTILITY type (X11).
    */
    enum class WindowKind : std::uint8_t { NORMAL, DIALOG, TOOL };

    /*
    ** Начальное место окна: решает ОС; по центру экрана (монитора); по центру окна-владельца.
    ** DEFAULT у окна с владельцем приложение заменяет на CENTER_OWNER.
    **
    ** The initial window place: the OS decides; at the screen (monitor) center; at the owner window center.
    ** The application replaces DEFAULT with CENTER_OWNER for a window with an owner.
    */
    enum class WindowPosition : std::uint8_t { DEFAULT, CENTER, CENTER_OWNER };
}

#endif
