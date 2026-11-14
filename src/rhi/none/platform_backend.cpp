#include "gran/rhi/backend_factory.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran::rhi
{
    /*
    ** Сборка без GPU ОС (NATIVE=0, см. src/platform/none/): бэкенда по умолчанию нет. Безоконное приложение
    ** берёт NullBackend само (BackendKind::NONE, application_graphics.cpp), сюда оно не приходит. Код 0 —
    ** «сырого» кода графического API нет. На macOS здесь появится Metal (шаг M5).
    **
    ** A build without the OS GPU (NATIVE=0, see src/platform/none/): there is no default backend. A windowless
    ** application takes NullBackend itself (BackendKind::NONE, application_graphics.cpp) and never comes here. Code 0 —
    ** there is no raw graphics API code. On macOS Metal will appear here (step M5).
    */
    std::unique_ptr<RenderBackend> createPlatformBackend(const BackendOptions& options)
    {
        (void)options;
        throw GpuException(ErrorCode::GPU,
            i18n::format(i18n::Message::PLATFORM_NOT_BUILT, {"rhi::createPlatformBackend"}), 0);
    }
}
