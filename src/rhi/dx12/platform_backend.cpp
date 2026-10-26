#include "gran/rhi/backend_factory.hpp"
#include "gran/rhi/dx12/dx12_backend.hpp"

namespace gran::rhi
{
    // На Windows бэкенд по умолчанию — Direct3D 12 / On Windows the default backend is Direct3D 12
    std::unique_ptr<RenderBackend> createPlatformBackend(const BackendOptions& options)
    {
        return createDx12Backend(options);
    }
}
