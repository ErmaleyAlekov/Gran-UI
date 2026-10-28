#include "gran/rhi/dx12/internal/dx12_internal.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <cstdlib>

namespace gran::rhi::dx12
{
    namespace
    {
        /*
        ** Отладочный слой D3D12 до создания устройства; GPU-Based Validation — по переменной
        ** окружения GRAN_GPU_VALIDATION=1 (медленно, ловит чтение за границами в шейдерах).
        **
        ** The D3D12 debug layer before the device is created; GPU-Based Validation — by the
        ** GRAN_GPU_VALIDATION=1 environment variable (slow, catches out-of-bounds reads in shaders).
        */
        bool enableDebugLayer()
        {
            ComRef<ID3D12Debug> debug;
            ComRef<ID3D12Debug1> validation;
            const char* gpuValidation = std::getenv("GRAN_GPU_VALIDATION");

            if (FAILED(D3D12GetDebugInterface(__uuidof(ID3D12Debug), reinterpret_cast<void**>(debug.put()))))
                return false;
            debug->EnableDebugLayer();
            bool wantValidation = gpuValidation != nullptr && gpuValidation[0] == '1';
            if (wantValidation && SUCCEEDED(debug->QueryInterface(__uuidof(ID3D12Debug1),
                reinterpret_cast<void**>(validation.put()))))
                validation->SetEnableGPUBasedValidation(TRUE);
            return true;
        }

        /*
        ** Устройство пригодно: шейдерная модель 6.0 и привязка ресурсов уровня 2
        ** («безлимитная» таблица текстур с динамической индексацией).
        **
        ** The device is suitable: shader model 6.0 and resource binding tier 2
        ** (a "bindless" texture table with dynamic indexing).
        */
        bool deviceSuitable(ID3D12Device* device)
        {
            D3D12_FEATURE_DATA_SHADER_MODEL model{D3D_SHADER_MODEL_6_0};
            D3D12_FEATURE_DATA_D3D12_OPTIONS options{};

            if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &model, sizeof(model))))
                return false;
            if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options))))
                return false;
            return model.HighestShaderModel >= D3D_SHADER_MODEL_6_0
                && options.ResourceBindingTier >= D3D12_RESOURCE_BINDING_TIER_2;
        }

        // Первый аппаратный адаптер по убыванию производительности, на котором создаётся пригодное устройство
        // The first hardware adapter by decreasing performance on which a suitable device is created
        bool pickHardware(DeviceContext& context)
        {
            ComRef<IDXGIFactory6> factory6;

            if (FAILED(context.factory->QueryInterface(__uuidof(IDXGIFactory6), reinterpret_cast<void**>(factory6.put()))))
                return false;
            for (UINT index = 0;; ++index)
            {
                ComRef<IDXGIAdapter1> adapter;
                DXGI_ADAPTER_DESC1 desc{};
                if (factory6->EnumAdapterByGpuPreference(index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                    __uuidof(IDXGIAdapter1), reinterpret_cast<void**>(adapter.put())) == DXGI_ERROR_NOT_FOUND)
                    return false;
                adapter->GetDesc1(&desc);
                if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
                    continue;
                if (SUCCEEDED(D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device),
                    reinterpret_cast<void**>(context.device.put()))) && deviceSuitable(context.device.get()))
                {
                    context.adapter = std::move(adapter);
                    return true;
                }
            }
        }

        // Программный адаптер WARP: медленно, но работает без видеокарты / The WARP software adapter: slow, but works without a video card
        void pickWarp(DeviceContext& context)
        {
            GRAN_CHECK_HR(context.factory->EnumWarpAdapter(__uuidof(IDXGIAdapter1),
                reinterpret_cast<void**>(context.adapter.put())));
            GRAN_CHECK_HR(D3D12CreateDevice(context.adapter.get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device),
                reinterpret_cast<void**>(context.device.put())));
            if (!deviceSuitable(context.device.get()))
                throw GpuException(ErrorCode::GPU, i18n::format(i18n::Message::GPU_NO_ADAPTER,
                    {"Direct3D 12 (SM 6.0, Resource Binding Tier 2)"}), 0);
        }
    }

    // Порядок: отладочный слой → фабрика → устройство → очередь; поддержка tearing — для VRR-мониторов
    // The order: the debug layer → the factory → the device → the queue; tearing support is for VRR monitors
    void createDeviceContext(bool debug, bool forceWarp, DeviceContext& context)
    {
        UINT factoryFlags = 0;
        BOOL tearing = FALSE;
        ComRef<IDXGIFactory5> factory5;
        D3D12_COMMAND_QUEUE_DESC queueDesc{};

        context.debug = debug && enableDebugLayer();
        if (context.debug)
            factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
        GRAN_CHECK_HR(CreateDXGIFactory2(factoryFlags, __uuidof(IDXGIFactory4), reinterpret_cast<void**>(context.factory.put())));
        if (forceWarp || !pickHardware(context))
            pickWarp(context);
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        GRAN_CHECK_HR(context.device->CreateCommandQueue(&queueDesc, __uuidof(ID3D12CommandQueue),
            reinterpret_cast<void**>(context.queue.put())));
        setDebugName(context.queue.get(), "gran.queue");
        if (SUCCEEDED(context.factory->QueryInterface(__uuidof(IDXGIFactory5), reinterpret_cast<void**>(factory5.put())))
            && SUCCEEDED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearing, sizeof(tearing))))
            context.tearing = tearing == TRUE;
    }
}
