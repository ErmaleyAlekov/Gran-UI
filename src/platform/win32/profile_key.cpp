#include "gran/platform/host.hpp"
#include <windows.h>

namespace gran::platform
{
    // Проверяем Shift при запуске, не создавая окна приложения.
    // Check Shift at startup without creating an application window.
    bool bypassProfileKey() noexcept
    {
        return (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    }
}
