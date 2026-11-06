#include "gran/rhi/dx12/internal/dx12_internal.hpp"

#include <cstdio>

namespace gran::rhi::dx12
{
    namespace
    {
        // Имя кода причины; незнакомый код — пустая строка / The name of a reason code; an unknown code is an empty string
        const char* lossName(HRESULT reason) noexcept
        {
            if (reason == DXGI_ERROR_DEVICE_HUNG)
                return "DXGI_ERROR_DEVICE_HUNG";
            if (reason == DXGI_ERROR_DEVICE_RESET)
                return "DXGI_ERROR_DEVICE_RESET";
            if (reason == DXGI_ERROR_DEVICE_REMOVED)
                return "DXGI_ERROR_DEVICE_REMOVED";
            if (reason == DXGI_ERROR_DRIVER_INTERNAL_ERROR)
                return "DXGI_ERROR_DRIVER_INTERNAL_ERROR";
            if (reason == DXGI_ERROR_INVALID_CALL)
                return "DXGI_ERROR_INVALID_CALL";
            if (reason == S_OK)
                return "S_OK";
            return "";
        }
    }

    /*
    ** Имя и шестнадцатеричный код: имя нужно человеку, код — для поиска в документации, если имени нет.
    ** S_OK тоже выводится: значит, устройство цело, а потерю сообщил другой вызов (Present, Close).
    **
    ** The name and the hexadecimal code: the name is for a human, the code is for a documentation search when
    ** there is no name. S_OK is printed too: it means the device is intact and another call (Present, Close)
    ** reported the loss.
    */
    std::string describeDeviceLoss(HRESULT reason)
    {
        char code[16];
        std::string name = lossName(reason);

        std::snprintf(code, sizeof(code), "0x%08lX", static_cast<unsigned long>(reason));
        if (name.empty())
            return code;
        return name + " (" + code + ")";
    }
}
