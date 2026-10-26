#include "gran/rhi/dx12/internal/dx12_internal.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <cstdio>
#include <string>

namespace gran::rhi::dx12
{
    // Сброс, удаление и зависание устройства — потеря / A reset, removal and hang of the device are a loss
    bool isDeviceLoss(HRESULT result) noexcept
    {
        return result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET
            || result == DXGI_ERROR_DEVICE_HUNG || result == DXGI_ERROR_DRIVER_INTERNAL_ERROR;
    }

    /*
    ** Успех — ничего; потеря устройства — GpuException(DEVICE_LOST), иначе — GpuException(GPU).
    ** В сообщении: имя вызова, код в шестнадцатеричном виде, файл и строка.
    **
    ** Success — nothing; a device loss — GpuException(DEVICE_LOST), otherwise — GpuException(GPU).
    ** The message holds the call name, the code in hexadecimal, the file and the line.
    */
    void checkResult(HRESULT result, const char* call, const char* file, int line)
    {
        char code[16];

        if (SUCCEEDED(result))
            return;
        std::snprintf(code, sizeof(code), "0x%08lX", static_cast<unsigned long>(result));
        ErrorCode category = ErrorCode::GPU;
        if (isDeviceLoss(result))
            category = ErrorCode::DEVICE_LOST;
        throw GpuException(category, i18n::format(i18n::Message::GPU_CALL_FAILED,
            {call, code, file, std::to_string(line)}), static_cast<long long>(result));
    }

    // Имя переводится в UTF-16, как требует SetName / The name is converted to UTF-16, as SetName requires
    void setDebugName(ID3D12Object* object, const char* name)
    {
        wchar_t wide[128];
        int length = MultiByteToWideChar(CP_UTF8, 0, name, -1, wide, 128);

        // Имя только для отладчиков: если перевести не удалось, объект остаётся безымянным
        // The name is only for debuggers: if the conversion fails the object stays unnamed
        if (object != nullptr && length > 0)
            object->SetName(wide);
    }

    // Переход ресурса между состояниями; одинаковые состояния — без барьера
    // A resource transition between states; equal states — no barrier
    void transition(ID3D12GraphicsCommandList* list, ID3D12Resource* resource, D3D12_RESOURCE_STATES before,
        D3D12_RESOURCE_STATES after)
    {
        D3D12_RESOURCE_BARRIER barrier{};

        if (before == after)
            return;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = resource;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = before;
        barrier.Transition.StateAfter = after;
        list->ResourceBarrier(1, &barrier);
    }

    // Номер слота и причина — в тексте из каталога / The slot number and the reason are in the catalog text
    void throwBadTexture(std::uint32_t slot, const char* reason)
    {
        throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::GPU_BAD_TEXTURE,
            {std::to_string(slot), reason}));
    }
}
