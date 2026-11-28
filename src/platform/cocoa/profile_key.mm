#include "gran/platform/host.hpp"
#import <AppKit/AppKit.h>

namespace gran::platform
{
    // Проверяем Shift при запуске, не создавая окна приложения.
    // Check Shift at startup without creating an application window.
    bool bypassProfileKey() noexcept
    {
        return ([NSEvent modifierFlags] & NSEventModifierFlagShift) != 0;
    }
}
