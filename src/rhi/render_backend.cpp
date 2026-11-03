#include "gran/rhi/render_backend.hpp"

namespace gran::rhi
{
    // Бэкенд без сведений о видеокарте (тестовые заглушки) / A backend without video card information (test stubs)
    AdapterInfo RenderBackend::adapterInfo() const
    {
        return AdapterInfo{};
    }
}
