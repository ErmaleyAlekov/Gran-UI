#ifndef GRAN_RHI_VULKAN_BACKEND_IMPL_HPP
#define GRAN_RHI_VULKAN_BACKEND_IMPL_HPP

#include "gran/rhi/vulkan/vulkan_backend.hpp"
#include "gran/rhi/vulkan/internal/owned.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/rhi/vulkan/internal/vulkan_api.hpp"
#include <array>
#include <algorithm>
#include <limits>
#include <mutex>
#include <type_traits>

namespace gran::rhi::vk
{
    // Полная нулевая инициализация обязательна для совместимости с разными версиями SDK.
    // Full zero initialization is required for compatibility across SDK versions.
    template<class T> T structure(VkStructureType type)
    {
        T value{};
        value.sType = type;
        return value;
    }
    // Vulkan использует указатели либо uint64_t в зависимости от разрядности платформы.
    // Vulkan uses pointers or uint64_t depending on the platform bitness.
    template<class T> std::uint64_t handleValue(T handle)
    {
        if constexpr (std::is_pointer<T>::value) return reinterpret_cast<std::uintptr_t>(handle);
        else return static_cast<std::uint64_t>(handle);
    }
    constexpr unsigned FRAME_COUNT = 2;
    constexpr unsigned BLUR_LEVELS = 6;
    constexpr unsigned MAX_TEXTURES = 4096;
    void check(VkResult result, const char* call, const char* file, int line);
    void invalid(const char* resource);

    // Память объявлена раньше ресурса: обратное разрушение освобождает ресурс первым.
    // Memory precedes its resource: reverse destruction releases the resource first.
    struct Buffer
    {
        Owned<VkDeviceMemory> memory;
        Owned<VkBuffer> handle;
        VkDeviceSize size = 0;
    };
    struct Image
    {
        Owned<VkDeviceMemory> memory;
        Owned<VkImage> handle;
        Owned<VkImageView> view;
        Owned<VkFramebuffer> framebuffer;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
        unsigned width = 0;
        unsigned height = 0;
        unsigned bytes = 4;
        // Содержимое ещё не определено: первая загрузка кадра очищает изображение нулями
        // The contents are not defined yet: the first frame upload clears the image with zeros
        bool cleared = false;
    };
    // Область updateTexture, ждущая кадра: её байты лежат плотно в _uploadBytes со смещения offset
    // An updateTexture region waiting for a frame: its bytes lie tightly in _uploadBytes from offset
    struct PendingUpload
    {
        unsigned slot = 0;
        TextureRegion region{};
        std::size_t offset = 0;
    };
    // Ресурсы кадра освобождаются лишь после его fence; семафоры показа принадлежат изображениям swapchain.
    // Frame resources are freed only after their fence; presentation semaphores belong to swapchain images.
    struct Frame
    {
        Owned<VkCommandPool> pool;
        Owned<VkDescriptorPool> descriptors;
        Owned<VkFence> fence;
        Owned<VkSemaphore> acquired;
        VkCommandBuffer commands = VK_NULL_HANDLE;
        // Постоянные буферы кадра растут по мере надобности; переиспользуются после его fence
        // The persistent frame buffers grow as needed; they are reused after its fence
        Buffer instances;
        Buffer clips;
        Buffer upload;
        std::vector<Image> retired;
        bool submitted = false;
    };
    // Контекст живёт дольше всех ресурсов; Backend ждёт очередь до начала обратного разрушения.
    // The context outlives all resources; Backend waits for the queue before reverse destruction starts.
    class VulkanBackend final : public RenderBackend, public SnapshotSource
    {
    public:
        explicit VulkanBackend(const BackendOptions& options);
        ~VulkanBackend() override;
        void resize(unsigned width, unsigned height) override;
        unsigned createTexture(const TextureDesc& desc) override;
        void updateTexture(unsigned slot, const TextureRegion& region, const std::uint8_t* pixels, unsigned pitch) override;
        void releaseTexture(unsigned slot) override;
        FrameStatus render(const FrameData& data) override;
        std::vector<std::string> takeDiagnostics() override;

        // ---- vulkan_adapter.cpp ----
        AdapterInfo adapterInfo() const override;
        std::string lostReason() const override;
        void waitPresented() override;
        void readPixels(std::vector<std::uint8_t>& rgba, unsigned& width, unsigned& height) override;
    private:
        void createInstance();
        void instanceLayers(std::vector<const char*>& extensions, const char*& validation);
        void createDebug();
        void nameObject(VkObjectType type, std::uint64_t handle, const char* name);
        void nameImage(Image& image, const char* name);
        void nameStatics();
        void createSurface();
        void createDevice();
        bool tryDevice(VkPhysicalDevice candidate);
        bool createLogical(VkPhysicalDevice candidate, unsigned family, bool presentWait);
        std::uint32_t instanceVersion() const;
        bool hasPresentWait(VkPhysicalDevice candidate) const;
        bool presentWaitSupported(VkPhysicalDevice candidate) const;
        bool queueFamily(VkPhysicalDevice candidate, unsigned& family);
        void createFrames();
        void createFrame(Frame& frame);
        void createLayouts();
        void createRenderPass();
        void createPipelines();
        void makePipeline(unsigned index, VkShaderModule vertex, VkShaderModule pixel);
        Owned<VkShaderModule> shader(const unsigned char* bytes, std::size_t size);
        unsigned memoryType(unsigned bits, VkMemoryPropertyFlags flags);
        Buffer makeBuffer(VkDeviceSize size, VkBufferUsageFlags usage);
        void writeBuffer(Buffer& buffer, const void* bytes, VkDeviceSize size);
        void ensureBuffer(Buffer& buffer, VkDeviceSize size, VkBufferUsageFlags usage);
        void clearTexture(Image& image);
        void copyRegion(const PendingUpload& upload);
        Image makeImage(const TextureDesc& desc, bool target);
        void imageView(Image& image, VkFormat format, bool target);
        void barrier(Image& image, VkImageLayout layout);
        void createTargets();
        void createSwapchain();
        VkSurfaceFormatKHR surfaceFormat();
        VkPresentModeKHR presentMode();
        void swapchainImages();
        bool acquire();
        FrameStatus present();
        void copyToSwapchain();
        void waitFrames();
        void beginFrame();
        void submit();
        void uploadTextures();
        void validate(const FrameData& data) const;
        void validateInstances(const FrameData& data) const;
        void drawFrame(const FrameData& data);
        void beginTarget(Image& image, bool clear, const float* color);
        void bind(const FrameData& data, unsigned image);
        VkDescriptorSet descriptors(const std::array<VkImageView, 4>& views);
        void drawSegment(const FrameData& data, const FrameSegment& segment);
        void drawBatch(const FrameData& data, unsigned first, unsigned count, unsigned image);
        void backdrop(float radius);
        void blur(unsigned source, unsigned destination, unsigned pipeline);
        render::FrameConstants constants(unsigned width, unsigned height) const;
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
            VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* data, void* user) noexcept;

        BackendOptions _options;
        std::mutex _diagnosticMutex;
        std::vector<std::string> _diagnostics;
        Owned<VkInstance> _instance;
        Owned<VkDebugUtilsMessengerEXT> _debug;
        Owned<VkSurfaceKHR> _surface;
        VkPhysicalDevice _physical = VK_NULL_HANDLE; // Заимствован у instance. / Borrowed from instance.
        unsigned _family = 0;
        VkPhysicalDeviceProperties _properties{};
        Owned<VkDevice> _device;
        VkQueue _queue = VK_NULL_HANDLE; // Заимствована у device. / Borrowed from device.
        Owned<VkRenderPass> _pass;
        Owned<VkDescriptorSetLayout> _descriptorLayout;
        Owned<VkPipelineLayout> _layout;
        Owned<VkSampler> _sampler;
        std::array<Owned<VkPipeline>, 3> _pipelines;
        Owned<VkSwapchainKHR> _swapchain;
        std::vector<VkImage> _swapImages; // Заимствованы у swapchain. / Borrowed from swapchain.
        std::vector<Owned<VkSemaphore>> _presented;
        std::array<Frame, FRAME_COUNT> _frames;
        std::vector<Image> _textures;
        // Загрузки между кадрами: копируются в буфер кадра после ожидания его fence
        // Uploads between frames: copied into the frame buffer after waiting for its fence
        std::vector<PendingUpload> _uploads;
        std::vector<std::uint8_t> _uploadBytes;
        Image _target;
        std::array<Image, BLUR_LEVELS> _blur;
        unsigned _current = 0;
        unsigned _swapIndex = 0;
        VkExtent2D _swapExtent{};
        bool _recreate = false;
        bool _hasFrame = false;
        bool _syncValidation = false;
        bool _debugEnabled = false;
        // Ожидание показа (vulkan_present_wait.cpp): версия экземпляра, включены ли расширения, vkWaitForPresentKHR,
        // номер последнего отправленного кадра и номер кадра, который ещё не дождались (0 — нечего ждать)
        // Waiting for presentation (vulkan_present_wait.cpp): the instance version, whether the extensions are on,
        // vkWaitForPresentKHR, the number of the last sent frame and the number of the frame not waited for yet (0 — none)
        std::uint32_t _apiVersion = VK_API_VERSION_1_0;
        bool _presentWait = false;
        PFN_vkVoidFunction _waitForPresent = nullptr;
        std::uint64_t _presentId = 0;
        std::uint64_t _waitId = 0;
    };
}
#define GRAN_CHECK_VK(call) ::gran::rhi::vk::check((call), #call, __FILE__, __LINE__)
#endif
