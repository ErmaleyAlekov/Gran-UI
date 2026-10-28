#include "gran/rhi/dx11/internal/dx11_backend_impl.hpp"

#include <cstdio>
#include <cstring>

namespace gran::rhi::dx11
{
    // Удаление, сброс и зависание требуют пересоздания бэкенда.
    // Removal, reset and a hang require recreating the backend.
    bool deviceLost(HRESULT result) noexcept
    {
        return result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET
            || result == DXGI_ERROR_DEVICE_HUNG || result == DXGI_ERROR_DRIVER_INTERNAL_ERROR;
    }

    // Сохраняем код драйвера и место неудачного вызова.
    // Preserve the driver code and the failed call location.
    void check(HRESULT result, const char* call, const char* file, int line)
    {
        char code[16];
        ErrorCode category = ErrorCode::GPU;

        if (SUCCEEDED(result))
            return;
        if (deviceLost(result))
            category = ErrorCode::DEVICE_LOST;
        std::snprintf(code, sizeof(code), "0x%08lX", static_cast<unsigned long>(result));
        throw GpuException(category, i18n::format(i18n::Message::GPU_CALL_FAILED,
            {call, code, file, std::to_string(line)}), result);
    }

    // Имена видны в PIX и отладочном слое.
    // Names are visible in PIX and the debug layer.
    void nameObject(ID3D11DeviceChild* object, const char* name)
    {
        if (object != nullptr && name != nullptr)
            GRAN_DX11_CHECK(object->SetPrivateData(WKPDID_D3DDebugObjectName,
                static_cast<UINT>(std::strlen(name)), name));
    }

    // Проверки размеров используют общий переведённый каталог.
    // Size validation uses the shared translated catalog.
    void invalid(const char* resource)
    {
        throw GpuException(ErrorCode::INVALID_ARGUMENT,
            i18n::format(i18n::Message::GPU_BAD_TEXTURE, {resource, "D3D11"}), 0);
    }
}
