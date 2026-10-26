#ifndef GRAN_RHI_BACKEND_FACTORY_HPP
#define GRAN_RHI_BACKEND_FACTORY_HPP

#include "gran/rhi/render_backend.hpp"

namespace gran::rhi
{
    /*
    ** Бэкенд по умолчанию для ОС сборки (паттерн Abstract Factory): Windows — Direct3D 12,
    ** Linux — Vulkan (сборка с VULKAN=1). Реализация лежит в каталоге своего бэкенда.
    **
    ** The default backend for the build OS (the Abstract Factory pattern): Windows — Direct3D 12,
    ** Linux — Vulkan (a build with VULKAN=1). The implementation lies in its backend directory.
    */
    std::unique_ptr<RenderBackend> createPlatformBackend(const BackendOptions& options);
}

#endif
