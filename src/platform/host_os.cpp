#include "gran/platform/host.hpp"

namespace gran::platform
{
    /*
    ** Единственное место общего кода платформы, где ОС выбирается препроцессором: остальные модули
    ** спрашивают hostOs() и не содержат своих #if по ОС для соглашений интерфейса.
    **
    ** The only place of the common platform code where the OS is chosen by the preprocessor: the other modules
    ** ask hostOs() and contain no #if by the OS of their own for the interface conventions.
    */
    HostOs hostOs() noexcept
    {
#if defined(_WIN32)
        return HostOs::WINDOWS;
#elif defined(__APPLE__)
        return HostOs::MACOS;
#else
        return HostOs::LINUX;
#endif
    }
}
