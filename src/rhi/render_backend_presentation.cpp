#include "gran/rhi/render_backend.hpp"

namespace gran::rhi
{
    // Базовый бэкенд не отслеживает показ кадров.
    // The default backend does not track frame presentation.
    bool RenderBackend::canConfirmPresentation() const
    {
        return false;
    }

    std::uint64_t RenderBackend::lastPresentation() const
    {
        return 0;
    }

    RenderBackend::Presentation RenderBackend::pollPresentation(std::uint64_t)
    {
        return Presentation::RETRY_FRAME;
    }
}
