#ifndef GRAN_PLATFORM_HOST_HPP
#define GRAN_PLATFORM_HOST_HPP

#include <cstdint>

namespace gran::platform
{
    /*
    ** Семейство ОС, под которую собран движок. Общий код спрашивает его только ради соглашений, которые
    ** видит пользователь (вид подписей сочетаний клавиш в меню), а не ради вызовов ОС: те — в platform/<ОС>/.
    ** LINUX — любая Unix-система с X11 (и Wayland позже), не только Linux.
    **
    ** The OS family the engine is built for. The common code asks for it only for the conventions the user
    ** sees (the look of the shortcut labels in menus), not for OS calls: those are in platform/<OS>/.
    ** LINUX is any Unix system with X11 (and Wayland later), not only Linux.
    */
    enum class HostOs : std::uint8_t { WINDOWS, LINUX, MACOS };

    // ОС сборки (host_os.cpp; выбор — при компиляции) / The build OS (host_os.cpp; chosen at compile time)
    HostOs hostOs() noexcept;
}

#endif
