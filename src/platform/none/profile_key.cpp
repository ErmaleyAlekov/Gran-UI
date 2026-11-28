#include "gran/platform/host.hpp"

namespace gran::platform
{
    // Проверяем Shift при запуске, не создавая окна приложения.
    // Check Shift at startup without creating an application window.
    bool bypassProfileKey() noexcept
    {
        return false;
    }
}
