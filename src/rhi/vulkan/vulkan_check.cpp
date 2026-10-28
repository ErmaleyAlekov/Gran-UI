#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"

namespace gran::rhi::vk
{
    // Числовой VkResult сохраняется для восстановления после DEVICE_LOST.
    // Preserve the numeric VkResult for recovery after DEVICE_LOST.
    void check(VkResult result, const char* call, const char* file, int line)
    {
        if (result == VK_SUCCESS)
            return;
        ErrorCode code = ErrorCode::GPU;
        if (result == VK_ERROR_DEVICE_LOST) code = ErrorCode::DEVICE_LOST;
        throw GpuException(code, i18n::format(i18n::Message::GPU_CALL_FAILED,
            {call, std::to_string(result), file, std::to_string(line)}), result);
    }

    // Ошибки входных данных не попадают в драйвер.
    // Invalid input never reaches the driver.
    void invalid(const char* resource)
    {
        throw GpuException(ErrorCode::GPU, i18n::format(i18n::Message::GPU_BAD_TEXTURE,
            {resource, i18n::format(i18n::Message::CAPI_ARGUMENT)}), 0);
    }
}
