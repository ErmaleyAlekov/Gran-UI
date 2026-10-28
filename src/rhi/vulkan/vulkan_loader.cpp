#define VK_USE_PLATFORM_XLIB_KHR
#include "gran/rhi/vulkan/internal/vulkan_api.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <dlfcn.h>
#include <mutex>

#define GRAN_VK_DEFINE(name) PFN_##name name = nullptr;
#define GRAN_VK_RESOLVE(name) name = reinterpret_cast<PFN_##name>(resolve(library, #name));

namespace gran::rhi::vk
{
    GRAN_VK_FUNCTIONS(GRAN_VK_DEFINE)
    PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR = nullptr;

    namespace
    {
        // Понятная ошибка вместо отказа динамического загрузчика: что не нашлось и какой пакет поставить
        // A clear error instead of a dynamic loader refusal: what is missing and which package to install
        [[noreturn]] void missingLoader(const char* detail)
        {
            throw GpuException(ErrorCode::GPU, i18n::format(i18n::Message::GPU_VULKAN_LOADER, {detail}), 0);
        }

        // Адрес функции загрузчика; функция ядра 1.0 есть в любом загрузчике, её отсутствие — испорченная установка
        // The address of a loader function; a 1.0 core function exists in any loader, its absence means a broken installation
        void* resolve(void* library, const char* name)
        {
            void* address = dlsym(library, name);

            if (address == nullptr)
                missingLoader(name);
            return address;
        }

        // Сначала имя с версией ABI (пакет libvulkan1), затем ссылка из пакета разработчика
        // First the name with the ABI version (the libvulkan1 package), then the link from the developer package
        void loadOnce()
        {
            void* library = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);

            if (library == nullptr)
                library = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
            if (library == nullptr)
                missingLoader("libvulkan.so.1");
            GRAN_VK_FUNCTIONS(GRAN_VK_RESOLVE)
            GRAN_VK_RESOLVE(vkCreateXlibSurfaceKHR)
        }
    }

    // call_once повторяет попытку, если прежняя закончилась исключением (пакет могли доустановить)
    // call_once retries if the former attempt ended with an exception (the package may have been installed meanwhile)
    void loadVulkan()
    {
        static std::once_flag loaded;

        std::call_once(loaded, loadOnce);
    }
}
