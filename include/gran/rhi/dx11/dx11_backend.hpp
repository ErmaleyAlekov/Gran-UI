#ifndef GRAN_RHI_DX11_BACKEND_HPP
#define GRAN_RHI_DX11_BACKEND_HPP

#include "gran/rhi/render_backend.hpp"

namespace gran::rhi
{
    // D3D11 FL11_0: окно или SnapshotSource без окна; аппаратный адаптер или WARP.
    // D3D11 FL11_0: a window or an offscreen SnapshotSource; hardware or WARP.
    std::unique_ptr<RenderBackend> createDx11Backend(const BackendOptions& options);
}

#endif
