#include "gran/app/app_driver.hpp"
#include "gran/app/internal/app_state.hpp"

namespace gran
{
    namespace
    {
        // Бэкенд окна по номеру; nullptr — нет окна или графики ещё нет / The window backend by number; nullptr — no window or no graphics yet
        rhi::RenderBackend* backendAt(const Application::State& state, std::size_t index) noexcept
        {
            if (index >= state.windows.size())
                return nullptr;
            return state.windows[index]->backend.get();
        }
    }

    // Читать умеют бэкенды вне экрана (D3D12, D3D11, Vulkan); пустой — нет / The offscreen backends can read (D3D12, D3D11, Vulkan); the empty one cannot
    bool AppDriver::readFrame(std::size_t index, std::vector<std::uint8_t>& rgba, std::uint32_t& width,
        std::uint32_t& height)
    {
        rhi::SnapshotSource* source = dynamic_cast<rhi::SnapshotSource*>(backendAt(state(), index));

        if (source == nullptr)
            return false;
        source->readPixels(rgba, width, height);
        return true;
    }

    const rhi::NullStats* AppDriver::nullStats(std::size_t index) const
    {
        const rhi::NullBackend* backend = dynamic_cast<const rhi::NullBackend*>(backendAt(state(), index));

        if (backend == nullptr)
            return nullptr;
        return &backend->stats();
    }
}
