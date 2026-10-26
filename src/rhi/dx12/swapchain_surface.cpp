#include "gran/rhi/dx12/internal/swapchain_surface.hpp"

#include <string>

namespace gran::rhi::dx12
{
    namespace
    {
        // Описание цепочки обмена / The swap chain description
        DXGI_SWAP_CHAIN_DESC1 chainDesc(const BackendOptions& options, UINT flags) noexcept
        {
            DXGI_SWAP_CHAIN_DESC1 desc{};

            desc.Width = options.width;
            desc.Height = options.height;
            desc.Format = TARGET_FORMAT;
            desc.SampleDesc.Count = 1;
            desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            desc.BufferCount = SWAP_BUFFERS;
            desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            desc.Flags = flags;
            return desc;
        }
    }

    // Цепочка для HWND; Alt+Enter отключён — полноэкранный режим решает приложение
    // A chain for the HWND; Alt+Enter is disabled — the application decides on fullscreen
    SwapchainSurface::SwapchainSurface(DeviceContext& context, const BackendOptions& options)
        : _tearing(context.tearing)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heap{};
        ComRef<IDXGISwapChain1> chain;
        HWND window = static_cast<HWND>(options.nativeWindow);

        if (_tearing)
            _flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        DXGI_SWAP_CHAIN_DESC1 desc = chainDesc(options, _flags);
        GRAN_CHECK_HR(context.factory->CreateSwapChainForHwnd(context.queue.get(), window, &desc, nullptr, nullptr,
            chain.put()));
        GRAN_CHECK_HR(chain->QueryInterface(__uuidof(IDXGISwapChain3), reinterpret_cast<void**>(_swapchain.put())));
        GRAN_CHECK_HR(context.factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));
        heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        heap.NumDescriptors = SWAP_BUFFERS;
        GRAN_CHECK_HR(context.device->CreateDescriptorHeap(&heap, __uuidof(ID3D12DescriptorHeap),
            reinterpret_cast<void**>(_rtvHeap.put())));
        _rtvSize = context.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        acquireBuffers(context.device.get());
    }

    // Ссылки на буферы и RTV для каждого / References to the buffers and an RTV for each
    void SwapchainSurface::acquireBuffers(ID3D12Device* device)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = _rtvHeap->GetCPUDescriptorHandleForHeapStart();

        for (UINT index = 0; index < SWAP_BUFFERS; ++index)
        {
            std::string name = "gran.backBuffer" + std::to_string(index);
            GRAN_CHECK_HR(_swapchain->GetBuffer(index, __uuidof(ID3D12Resource),
                reinterpret_cast<void**>(_buffers[index].put())));
            setDebugName(_buffers[index].get(), name.c_str());
            device->CreateRenderTargetView(_buffers[index].get(), nullptr, handle);
            handle.ptr += _rtvSize;
        }
    }

    // Ссылки отпускаются, затем ResizeBuffers (GPU простаивает — гарантирует бэкенд)
    // The references are released, then ResizeBuffers (the GPU is idle — the backend guarantees it)
    void SwapchainSurface::resize(ID3D12Device* device, std::uint32_t width, std::uint32_t height)
    {
        for (ComRef<ID3D12Resource>& buffer : _buffers)
            buffer.reset();
        GRAN_CHECK_HR(_swapchain->ResizeBuffers(SWAP_BUFFERS, width, height, TARGET_FORMAT, _flags));
        acquireBuffers(device);
    }

    // Фабрика поверхности окна / The window surface factory
    std::unique_ptr<Surface> createSwapchainSurface(DeviceContext& context, const BackendOptions& options)
    {
        return std::make_unique<SwapchainSurface>(context, options);
    }
}
