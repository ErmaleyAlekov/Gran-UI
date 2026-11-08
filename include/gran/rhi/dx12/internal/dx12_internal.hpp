#ifndef GRAN_RHI_DX12_INTERNAL_DX12_INTERNAL_HPP
#define GRAN_RHI_DX12_INTERNAL_DX12_INTERNAL_HPP

#include "gran/rhi/dx12/internal/com_ref.hpp"
#include "gran/rhi/render_backend.hpp"

// Без макросов min/max из <windows.h>: они ломают std::min/std::max (MSVC) / Without the min/max macros of <windows.h>: they break std::min/std::max (MSVC)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d12.h>
#include <dxgi1_6.h>

#include <cstdint>
#include <string>
#include <vector>

/*
** Проверка HRESULT: неудача → GpuException (потеря устройства — с кодом DEVICE_LOST).
** Checking an HRESULT: a failure → GpuException (a device loss — with the DEVICE_LOST code).
*/
#define GRAN_CHECK_HR(expression) \
    ::gran::rhi::dx12::checkResult((expression), #expression, __FILE__, __LINE__)

namespace gran::rhi::dx12
{
    // ---- dxgi_adapter.cpp ----

    // Имя адаптера DXGI (UTF-8), версия драйвера пользовательского режима и признак программного адаптера
    // The DXGI adapter name (UTF-8), the user mode driver version and the software adapter flag
    AdapterInfo describeDxgiAdapter(IDXGIAdapter* adapter, const char* api);

    // Кадров в полёте: CPU готовит следующий, пока GPU рисует текущий / Frames in flight: the CPU prepares the next one while the GPU draws the current
    constexpr std::uint32_t FRAMES_IN_FLIGHT = 2;

    // Буферов цепочки обмена / The swap chain buffer count
    constexpr std::uint32_t SWAP_BUFFERS = 3;

    // Слотов текстур в таблице дескрипторов (совпадает с границей корневой сигнатуры)
    // Texture slots in the descriptor table (matches the root signature bound)
    constexpr std::uint32_t TEXTURE_SLOTS = 256;

    // Уровней цепочки размытия подложки (копия кадра + 6 уменьшений) и их слоты в конце таблицы
    // The backdrop blur chain levels (the frame copy + 6 halvings) and their slots at the table end
    constexpr std::uint32_t BLUR_LEVELS = 7;
    constexpr std::uint32_t BLUR_SLOT_FIRST = TEXTURE_SLOTS - BLUR_LEVELS;

    // Формат цели: sRGB-значения без преобразования (смешивание как в браузерах)
    // The target format: sRGB values without conversion (blending as in browsers)
    constexpr DXGI_FORMAT TARGET_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;

    // ---- dx12_check.cpp ----

    // Неудачный HRESULT → исключение с текстом из каталога / A failed HRESULT → an exception with a catalog text
    void checkResult(HRESULT result, const char* call, const char* file, int line);

    // Имя объекта для PIX и отладочного слоя / An object name for PIX and the debug layer
    void setDebugName(ID3D12Object* object, const char* name);

    // Потеря ли устройства этот код / Whether this code is a device loss
    bool isDeviceLoss(HRESULT result) noexcept;

    // ---- dxgi_lost.cpp ----

    // Код из GetDeviceRemovedReason для журнала: «DXGI_ERROR_DEVICE_HUNG (0x887A0006)»; общий для D3D12 и D3D11
    // The GetDeviceRemovedReason code for the log: "DXGI_ERROR_DEVICE_HUNG (0x887A0006)"; shared by D3D12 and D3D11
    std::string describeDeviceLoss(HRESULT reason);

    // Неверная операция с текстурой → GranException(INVALID_ARGUMENT) / An invalid texture operation → GranException(INVALID_ARGUMENT)
    [[noreturn]] void throwBadTexture(std::uint32_t slot, const char* reason);

    // Барьер перехода состояния ресурса (все подресурсы) / A resource state transition barrier (all subresources)
    void transition(ID3D12GraphicsCommandList* list, ID3D12Resource* resource, D3D12_RESOURCE_STATES before,
        D3D12_RESOURCE_STATES after);

    /*
    ** Устройство и его спутники. Порядок полей — порядок создания; уничтожаются они
    ** в обратном порядке: очередь раньше устройства, устройство раньше фабрики.
    **
    ** The device and its companions. The field order is the creation order; they are destroyed
    ** in reverse order: the queue before the device, the device before the factory.
    */
    struct DeviceContext
    {
        ComRef<IDXGIFactory4> factory;
        ComRef<IDXGIAdapter1> adapter;
        ComRef<ID3D12Device> device;
        ComRef<ID3D12CommandQueue> queue;
        bool tearing = false;
        bool debug = false;
        // Способ показа окна для журнала (ставит цепочка окна) / The window presenting way for the log (set by the window chain)
        const char* present = "";
    };

    // ---- dx12_device.cpp ----

    // Фабрика, адаптер (высокопроизводительный или WARP), устройство и очередь
    // The factory, the adapter (high performance or WARP), the device and the queue
    void createDeviceContext(bool debug, bool forceWarp, DeviceContext& context);

    /*
    ** Забор (fence) с событием ОС. Значение растёт на каждом signal; wait блокирует поток,
    ** пока GPU не дойдёт до значения. При потерянном устройстве завершённое значение —
    ** UINT64_MAX, поэтому ожидание не зависает.
    **
    ** A fence with an OS event. The value grows on each signal; wait blocks the thread
    ** until the GPU reaches the value. With a lost device the completed value is
    ** UINT64_MAX, so waiting never hangs.
    */
    class FenceSync
    {
    public:
        FenceSync() = default;
        ~FenceSync();
        FenceSync(const FenceSync&) = delete;
        FenceSync& operator=(const FenceSync&) = delete;

        // Создание забора и события / Creating the fence and the event
        void init(ID3D12Device* device);

        // Сигнал очереди со следующим значением; возвращает его / A queue signal with the next value; returns it
        std::uint64_t signal(ID3D12CommandQueue* queue);

        // Ждать, пока GPU пройдёт значение / Wait until the GPU passes the value
        void wait(std::uint64_t value);

        // Последнее пройденное GPU значение / The last value passed by the GPU
        std::uint64_t completed() const;

    private:
        ComRef<ID3D12Fence> _fence;
        HANDLE _event = nullptr;
        std::uint64_t _value = 0;
    };

    /*
    ** Очередь отложенного удаления: объект живёт, пока GPU не пройдёт значение забора
    ** кадра, в котором его использовали последним. Слот текстуры возвращается так же.
    **
    ** The deferred deletion queue: an object lives until the GPU passes the fence value
    ** of the frame that used it last. A texture slot is returned the same way.
    */
    class DeferredReleases
    {
    public:
        // Отложить освобождение объекта (и слота, если slot != NO_SLOT) / Defer releasing an object (and a slot if slot != NO_SLOT)
        void push(ComRef<ID3D12Resource> resource, std::uint32_t slot, std::uint64_t fenceValue);

        // Освободить пройденные; освобождённые слоты дописываются в freedSlots
        // Release the passed ones; the freed slots are appended to freedSlots
        void collect(std::uint64_t completedValue, std::vector<std::uint32_t>& freedSlots);

        // Слот не связан с объектом / The slot is not bound to the object
        static constexpr std::uint32_t NO_SLOT = 0xFFFFFFFFu;

    private:
        struct Entry
        {
            ComRef<ID3D12Resource> resource;
            std::uint32_t slot;
            std::uint64_t fenceValue;
        };

        std::vector<Entry> _entries;
    };

    // Выделение в буфере загрузки / An allocation in the upload buffer
    struct UploadAllocation
    {
        std::uint8_t* cpu = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS gpu = 0;
        ID3D12Resource* resource = nullptr;
        std::uint64_t offset = 0;
    };

    /*
    ** Кольцевой буфер загрузки одного кадра (UPLOAD-куча, постоянно отображён). Сбрасывается
    ** в начале кадра — после ожидания забора этого кадра, поэтому GPU его уже не читает.
    ** Память комбинированной записи: только последовательная запись, никакого чтения.
    **
    ** The upload ring buffer of one frame (the UPLOAD heap, persistently mapped). It is reset
    ** at the start of the frame — after waiting for this frame's fence, so the GPU no longer reads it.
    ** Write-combined memory: sequential writes only, no reading.
    */
    class UploadRing
    {
    public:
        UploadRing() = default;
        ~UploadRing();
        UploadRing(const UploadRing&) = delete;
        UploadRing& operator=(const UploadRing&) = delete;

        // Ёмкость не меньше minimum (рост степенями двойки), смещение — 0 / The capacity is at least minimum (growing by powers of two), the offset is 0
        void prepare(ID3D12Device* device, std::uint64_t minimum, const char* name);

        // Выделение с выравниванием; false — места нет (ошибка расчёта ёмкости) / An aligned allocation; false — no room (a capacity calculation error)
        bool allocate(std::uint64_t size, std::uint64_t alignment, UploadAllocation& allocation);

    private:
        // Освобождение буфера с отменой отображения / Releasing the buffer with unmapping
        void release() noexcept;

        ComRef<ID3D12Resource> _buffer;
        std::uint8_t* _mapped = nullptr;
        std::uint64_t _capacity = 0;
        std::uint64_t _offset = 0;
    };

    // Ожидающая загрузка части текстуры (пиксели — плотные строки) / A pending upload of a texture part (pixels are packed rows)
    struct PendingUpload
    {
        std::uint32_t slot;
        TextureRegion region;
        std::vector<std::uint8_t> pixels;
    };

    // Текстура таблицы и её текущее состояние / A table texture and its current state
    struct TextureEntry
    {
        ComRef<ID3D12Resource> resource;
        TextureDesc desc;
        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COPY_DEST;
    };

    /*
    ** Таблица текстур: видимая шейдеру куча дескрипторов на TEXTURE_SLOTS слотов, ресурсы
    ** и очередь загрузок. Пустые слоты заполнены нулевыми SRV: выборка из них даёт 0, а не сбой.
    **
    ** The texture table: a shader-visible descriptor heap of TEXTURE_SLOTS slots, the resources
    ** and the upload queue. Empty slots hold null SRVs: sampling them gives 0, not a failure.
    */
    class TextureTable
    {
    public:
        // Куча дескрипторов и нулевые SRV / The descriptor heap and null SRVs
        void init(ID3D12Device* device);

        // Ресурс в первом свободном слоте (слот TEXTURE_BACKDROP зарезервирован) / A resource in the first free slot (the TEXTURE_BACKDROP slot is reserved)
        std::uint32_t create(ID3D12Device* device, const TextureDesc& desc);

        // Проверка границ и копия пикселей в очередь / A bounds check and a copy of the pixels into the queue
        void update(std::uint32_t slot, const TextureRegion& region, const std::uint8_t* pixels, std::uint32_t rowPitch);

        // Ресурс уходит в отложенное удаление / The resource goes to deferred deletion
        void release(std::uint32_t slot, DeferredReleases& releases, std::uint64_t fenceValue);

        // Слоты, освобождённые после прохода забора: нулевые SRV и возврат в свободные
        // The slots freed after the fence passes: null SRVs and a return to the free ones
        void reclaim(ID3D12Device* device, const std::vector<std::uint32_t>& slots);

        // ---- texture_upload.cpp ----

        // Байтов буфера загрузки нужно на очередь загрузок / The upload buffer bytes needed for the upload queue
        std::uint64_t pendingBytes() const;

        /*
        ** Копирование очереди в текстуры, затем все текстуры в COPY_DEST (и новые без загрузок —
        ** созданные ресурсы обнулены) переводятся в состояние чтения шейдером.
        **
        ** Copying the queue into the textures, then all textures in COPY_DEST (new ones without uploads
        ** too — created resources are zeroed) are moved into the shader read state.
        */
        void recordUploads(ID3D12GraphicsCommandList* list, UploadRing& ring);

        // Куча для SetDescriptorHeaps и таблица для корневого параметра / The heap for SetDescriptorHeaps and the table for the root parameter
        ID3D12DescriptorHeap* heap() const noexcept;

        // SRV зарезервированного слота (подложка и цепочка размытия) / The SRV of a reserved slot (the backdrop and the blur chain)
        void bindReserved(ID3D12Device* device, std::uint32_t slot, ID3D12Resource* resource);

        // Число занятых слотов (граница для шейдера) / The number of used slots (the bound for the shader)
        std::uint32_t usedSlots() const noexcept;

    private:
        // Запись SRV слота (nullptr — нулевой дескриптор) / Writing a slot SRV (nullptr — a null descriptor)
        void writeView(ID3D12Device* device, std::uint32_t slot, ID3D12Resource* resource, DXGI_FORMAT format);

        // Загрузка одной области / Uploading one region
        void recordOne(ID3D12GraphicsCommandList* list, UploadRing& ring, const PendingUpload& upload);

        ComRef<ID3D12DescriptorHeap> _heap;
        std::uint32_t _increment = 0;
        std::vector<TextureEntry> _entries;
        std::vector<bool> _used;
        std::vector<PendingUpload> _pending;
    };

    // Корневая сигнатура и состояния конвейера / The root signature and the pipeline states
    struct PipelineSet
    {
        ComRef<ID3D12RootSignature> root;
        ComRef<ID3D12PipelineState> ui;
        ComRef<ID3D12PipelineState> blurDown;
        ComRef<ID3D12PipelineState> blurUp;
    };

    /*
    ** Цепочка размытия подложки: уровень 0 — копия кадра, далее уменьшения вдвое. Состояния
    ** уровней отслеживаются явно (правило 2 gpu_rules.md). Пересоздаётся при смене размера цели.
    **
    ** The backdrop blur chain: level 0 is a frame copy, then halvings. The level states
    ** are tracked explicitly (rule 2 of gpu_rules.md). It is recreated when the target size changes.
    */
    struct BackdropChain
    {
        ComRef<ID3D12DescriptorHeap> rtvHeap;
        ComRef<ID3D12Resource> levels[BLUR_LEVELS];
        D3D12_RESOURCE_STATES states[BLUR_LEVELS] = {};
        std::uint32_t rtvSize = 0;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
    };

    // RTV уровня цепочки (dx12_backdrop_chain.cpp) / The RTV of a chain level (dx12_backdrop_chain.cpp)
    D3D12_CPU_DESCRIPTOR_HANDLE backdropView(const BackdropChain& chain, std::uint32_t level) noexcept;

    // ---- dx12_pipeline.cpp ----

    // Корневая сигнатура: константы кадра, SRV экземпляров и отсечений, таблица текстур, сэмплер
    // The root signature: the frame constants, the instance and clip SRVs, the texture table, the sampler
    void createPipelines(ID3D12Device* device, PipelineSet& pipelines);

    // ---- dx12_pipeline_blur.cpp ----

    // Конвейеры уменьшения и увеличения двойного Kawase (без смешивания) / The dual Kawase down and up pipelines (without blending)
    void createBlurPipelines(ID3D12Device* device, PipelineSet& pipelines);

    /*
    ** Поверхность вывода (паттерн Strategy): цепочка обмена окна или текстура вне экрана.
    ** Состояние покоя поверхности — PRESENT (окно) или COPY_SOURCE (вне экрана, для чтения).
    **
    ** The output surface (the Strategy pattern): a window swap chain or an offscreen texture.
    ** The surface rest state is PRESENT (a window) or COPY_SOURCE (offscreen, for reading back).
    */
    class Surface
    {
    public:
        virtual ~Surface() = default;

        // Новый размер; вызывать только при простое GPU / A new size; call only when the GPU is idle
        virtual void resize(ID3D12Device* device, std::uint32_t width, std::uint32_t height) = 0;

        // Текущая цель и её RTV / The current target and its RTV
        virtual ID3D12Resource* target() = 0;
        virtual D3D12_CPU_DESCRIPTOR_HANDLE view() = 0;

        // Состояние покоя цели / The target rest state
        virtual D3D12_RESOURCE_STATES restState() const = 0;

        // Действия после отрисовки в том же списке команд (копия для чтения) / Actions after drawing in the same command list (a copy for reading)
        virtual void afterDraw(ID3D12GraphicsCommandList* list) = 0;

        // Показ кадра / Presenting the frame
        virtual HRESULT present(bool vsync) = 0;
    };

    // ---- swapchain_surface.cpp / offscreen_surface.cpp ----

    // Поверхность окна / A window surface
    std::unique_ptr<Surface> createSwapchainSurface(DeviceContext& context, const BackendOptions& options);

    // Поверхность вне экрана (класс OffscreenSurface, offscreen_surface.hpp) / An offscreen surface (the OffscreenSurface class, offscreen_surface.hpp)
    std::unique_ptr<Surface> createOffscreenSurface(ID3D12Device* device, std::uint32_t width, std::uint32_t height);

    // ---- dx12_debug.cpp ----

    // Сообщения отладочного слоя уровня ERROR и CORRUPTION (и очистка очереди) / The debug layer ERROR and CORRUPTION messages (and clearing the queue)
    std::vector<std::string> drainDebugMessages(ID3D12Device* device);
}

#endif
