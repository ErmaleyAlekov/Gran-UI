#include "gran/rhi/dx12/internal/dx12_backend_impl.hpp"

namespace gran::rhi::dx12
{
    // Адаптер, на котором создано устройство (железо или WARP) / The adapter the device is created on (hardware or WARP)
    AdapterInfo Dx12Backend::adapterInfo() const
    {
        return describeDxgiAdapter(_context.adapter.get(), "Direct3D 12");
    }
}
