#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran::app
{
    // Проверяются и живые окна, чтобы тесты перезагрузки не скрывали ошибки GPU.
    // Live windows are checked too, so reload tests cannot hide GPU errors.
    void checkGraphicsDiagnostics(AppWindow& win)
    {
        if (!win.shared.options.debugLayer)
            return;
        auto messages = win.backend->takeDiagnostics();
        if (!messages.empty())
            throw GpuException(ErrorCode::GPU,
                i18n::format(i18n::Message::GPU_DEBUG_MESSAGE, {"Application", messages.front()}), 0);
    }
}
