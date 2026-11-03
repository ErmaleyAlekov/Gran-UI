#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"

#include <string>

namespace gran::rhi::vk
{
    /*
    ** Имя и версия драйвера из свойств физического устройства. Кодировка driverVersion у производителей
    ** разная (у NVIDIA своя), поэтому выводится общая раскладка major.minor.patch и само число.
    **
    ** The name and the driver version from the physical device properties. The driverVersion encoding differs
    ** between vendors (NVIDIA has its own), so the common major.minor.patch layout and the number itself are printed.
    */
    AdapterInfo VulkanBackend::adapterInfo() const
    {
        AdapterInfo info;
        std::uint32_t version = _properties.driverVersion;

        info.api = "Vulkan";
        info.name = _properties.deviceName;
        info.driver = std::to_string(version >> 22) + "." + std::to_string((version >> 12) & 0x3FFu) + "."
            + std::to_string(version & 0xFFFu) + " (" + std::to_string(version) + ")";
        info.software = _properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU;
        return info;
    }
}
