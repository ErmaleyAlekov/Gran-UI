#ifndef GRAN_RHI_VULKAN_BACKEND_HPP
#define GRAN_RHI_VULKAN_BACKEND_HPP
#include "gran/rhi/render_backend.hpp"

namespace gran::rhi
{
    // Vulkan 1.0: GPU или программное CPU-устройство Mesa через системный loader.
    // Vulkan 1.0: a GPU or Mesa software CPU device through the system loader.
    std::unique_ptr<RenderBackend> createVulkanBackend(const BackendOptions& options);
}
#endif
