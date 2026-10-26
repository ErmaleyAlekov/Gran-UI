#ifndef GRAN_RHI_DX12_DX12_BACKEND_HPP
#define GRAN_RHI_DX12_DX12_BACKEND_HPP

#include "gran/rhi/render_backend.hpp"

namespace gran::rhi
{
    /*
    ** Бэкенд Direct3D 12 (только Windows). С окном (nativeWindow = HWND) рисует в цепочку
    ** обмена, без окна — в текстуру вне экрана и реализует SnapshotSource. Нет подходящей
    ** видеокарты — используется программный адаптер WARP (тесты на машинах без GPU).
    **
    ** The Direct3D 12 backend (Windows only). With a window (nativeWindow = HWND) it draws into
    ** a swap chain, without one — into an offscreen texture and implements SnapshotSource. Without
    ** a suitable video card the WARP software adapter is used (tests on machines without a GPU).
    */
    std::unique_ptr<RenderBackend> createDx12Backend(const BackendOptions& options);
}

#endif
