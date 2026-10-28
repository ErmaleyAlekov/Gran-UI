#ifndef GRAN_PLATFORM_WINDOW_FRAME_HPP
#define GRAN_PLATFORM_WINDOW_FRAME_HPP

#include <cstdint>

namespace gran::platform
{
    // Заголовок ОС по умолчанию; свой заголовок занимает клиентскую область.
    // The OS title bar is the default; a custom title bar occupies the client area.
    enum class TitleBar : std::uint8_t { SYSTEM, CUSTOM };

    // Роль участка разметки в управлении окном; NONE оставляет обычный ввод.
    // The role of a markup region in window management; NONE keeps ordinary input.
    enum class WindowRole : std::uint8_t { NONE, DRAG, MINIMIZE, MAXIMIZE, CLOSE };
}

#endif
