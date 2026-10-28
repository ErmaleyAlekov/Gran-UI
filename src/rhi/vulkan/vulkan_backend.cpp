#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"

namespace gran::rhi
{
    // Фабрика сохраняет общий интерфейс и для снимков без оконной системы.
    // The factory preserves the common interface for snapshots without a window system too.
    std::unique_ptr<RenderBackend> createVulkanBackend(const BackendOptions& options)
    {
        return std::make_unique<vk::VulkanBackend>(options);
    }
}

namespace gran::rhi::vk
{
    // Частично созданные ресурсы также освобождаются RAII при исключении.
    // RAII also releases partially created resources on an exception.
    VulkanBackend::VulkanBackend(const BackendOptions& options) : _options(options)
    {
#ifdef GRAN_DEBUG
        _options.debugLayer = true;
#endif
        if (options.width == 0 || options.height == 0 || options.width > 16384 || options.height > 16384)
            invalid("viewport");
        // Системный загрузчик Vulkan подключается здесь, а не при запуске программы
        // The system Vulkan loader is attached here, not at program start
        loadVulkan();
        createInstance();
        createDebug();
        createSurface();
        createDevice();
        createRenderPass();
        createLayouts();
        createPipelines();
        createFrames();
        createTargets();
        createSwapchain();
        nameStatics();
    }

    // Ошибку DEVICE_LOST при разрушении уже невозможно исправить; освобождаем все дескрипторы.
    // DEVICE_LOST during destruction cannot be recovered here; release all handles anyway.
    VulkanBackend::~VulkanBackend()
    {
        if (_device.get() != VK_NULL_HANDLE)
            (void)vkDeviceWaitIdle(_device.get());
    }

    // Смена размера ждёт все кадры и очередь показа, затем заменяет цели.
    // Resizing waits for every frame and the presentation queue, then replaces targets.
    void VulkanBackend::resize(unsigned width, unsigned height)
    {
        if (width == _options.width && height == _options.height && !_recreate)
            return;
        if (width > 16384 || height > 16384)
            invalid("viewport");
        waitFrames();
        _options.width = width;
        _options.height = height;
        _hasFrame = false;
        if (width == 0 || height == 0)
            return;
        createTargets();
        createSwapchain();
        _recreate = false;
    }
}
