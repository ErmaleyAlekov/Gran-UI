#include "gran/rhi/render_backend.hpp"

namespace gran::rhi
{
    // Бэкенд без сведений о видеокарте (тестовые заглушки) / A backend without video card information (test stubs)
    AdapterInfo RenderBackend::adapterInfo() const
    {
        return AdapterInfo{};
    }

    // Бэкенд, не умеющий назвать причину потери (тестовые заглушки) / A backend unable to name the loss reason (test stubs)
    std::string RenderBackend::lostReason() const
    {
        return std::string();
    }

    // Бэкенд без ожидания показа: кадр считается показанным сразу / A backend without presentation waiting: the frame counts as shown at once
    void RenderBackend::waitPresented()
    {
    }
}
