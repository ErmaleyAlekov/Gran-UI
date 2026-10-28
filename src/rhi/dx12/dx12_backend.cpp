#include "gran/rhi/dx12/internal/dx12_backend_impl.hpp"
#include "gran/rhi/dx12/dx12_backend.hpp"
#include "gran/core/gran_exception.hpp"

#include <string>

namespace gran::rhi::dx12
{
    namespace
    {
        // Распределитель команд кадра / The frame command allocator
        void createAllocator(ID3D12Device* device, FrameContext& frame, std::uint32_t index)
        {
            std::string name = "gran.allocator" + std::to_string(index);

            GRAN_CHECK_HR(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                __uuidof(ID3D12CommandAllocator), reinterpret_cast<void**>(frame.allocator.put())));
            setDebugName(frame.allocator.get(), name.c_str());
        }
    }

    /*
    ** Порядок создания совпадает с порядком полей. Если что-то бросит исключение, уже созданные
    ** поля разрушатся в обратном порядке; GPU в этот момент ничего не выполняет.
    **
    ** The creation order matches the field order. If something throws, the fields already created
    ** are destroyed in reverse order; the GPU executes nothing at that moment.
    */
    Dx12Backend::Dx12Backend(const BackendOptions& options)
        : _options(options), _offscreen(options.nativeWindow == nullptr)
    {
        createDeviceContext(options.debugLayer, options.forceWarp, _context);
        ID3D12Device* device = _context.device.get();
        _fence.init(device);
        createPipelines(device, _pipelines);
        createBlurPipelines(device, _pipelines);
        _textures.init(device);
        for (std::uint32_t index = 0; index < FRAMES_IN_FLIGHT; ++index)
            createAllocator(device, _frames[index], index);
        GRAN_CHECK_HR(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _frames[0].allocator.get(),
            nullptr, __uuidof(ID3D12GraphicsCommandList), reinterpret_cast<void**>(_list.put())));
        setDebugName(_list.get(), "gran.commandList");
        GRAN_CHECK_HR(_list->Close());
        if (_offscreen)
            _surface = createOffscreenSurface(device, options.width, options.height);
        else
            _surface = createSwapchainSurface(_context, options);
    }

    // Сначала простой GPU, затем поля разрушаются в обратном порядке; деструктор не бросает
    // The GPU idles first, then the fields are destroyed in reverse order; the destructor does not throw
    Dx12Backend::~Dx12Backend()
    {
        try
        {
            waitIdle();
        }
        catch (const GranException&)
        {
            // Устройство потеряно: GPU уже ничего не выполняет, ресурсы можно освобождать
            // The device is lost: the GPU executes nothing anymore, the resources can be released
        }
    }

    // Сигнал вне кадров и ожидание его прохода / A signal outside the frames and waiting for it to pass
    void Dx12Backend::waitIdle()
    {
        _lastSignaled = _fence.signal(_context.queue.get());
        _fence.wait(_lastSignaled);
    }

    // Нулевой размер (свёрнутое окно) пропускается: цепочка обмена не бывает пустой.
    // Цепочка размытия сбрасывается после простоя GPU и пересоздаётся при следующей подложке.
    // A zero size (a minimized window) is skipped: a swap chain is never empty.
    // The blur chain is dropped after the GPU idles and recreated at the next backdrop.
    void Dx12Backend::resize(std::uint32_t width, std::uint32_t height)
    {
        if (width == 0 || height == 0 || (width == _options.width && height == _options.height))
            return;
        waitIdle();
        _backdrop = BackdropChain{};
        _surface->resize(_context.device.get(), width, height);
        _options.width = width;
        _options.height = height;
    }
}

namespace gran::rhi
{
    // Фабрика бэкенда D3D12 / The D3D12 backend factory
    std::unique_ptr<RenderBackend> createDx12Backend(const BackendOptions& options)
    {
        return std::make_unique<dx12::Dx12Backend>(options);
    }
}
