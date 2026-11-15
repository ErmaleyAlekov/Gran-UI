#include "gran/rhi/backend_factory.hpp"
#include "gran/rhi/metal/metal_backend.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran::rhi
{
    // macOS: единственный API — Metal; явный запрос Direct3D или Vulkan — ошибка, а не тихая подмена
    // macOS: the only API is Metal; an explicit Direct3D or Vulkan request is an error, not a silent substitution
    std::unique_ptr<RenderBackend> createPlatformBackend(const BackendOptions& options)
    {
        if (options.backend != BackendKind::AUTO && options.backend != BackendKind::METAL)
            throw GpuException(ErrorCode::GPU, i18n::format(i18n::Message::GPU_NO_ADAPTER, {"Direct3D/Vulkan (macOS)"}), 0);
        return createMetalBackend(options);
    }
}
