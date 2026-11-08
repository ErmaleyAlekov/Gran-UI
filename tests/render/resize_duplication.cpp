#include "resize_duplication.hpp"
#include "gran/rhi/dx12/internal/com_ref.hpp"

#include <d3d11.h>
#include <dxgi1_2.h>

namespace render_tests
{
    namespace
    {
        using gran::rhi::dx12::ComRef;

        // Кадр «с мусором» — хотя бы 3 столбца полосы, больше чем наполовину белые или чёрные
        // A frame "with garbage" — at least 3 band columns that are more than half white or black
        constexpr int BAD_FRAME_COLUMNS = 3;
        constexpr UINT ACQUIRE_TIMEOUT_MS = 20;

        // Дублирование одного монитора и текстура для чтения полосы / The duplication of one monitor and a texture for reading the band
        struct Duplication
        {
            ComRef<ID3D11Device> device;
            ComRef<ID3D11DeviceContext> context;
            ComRef<IDXGIOutputDuplication> output;
            ComRef<ID3D11Texture2D> staging;
            D3D11_BOX box{};
        };

        // Устройство на адаптере монитора, дублирование и staging-текстура полосы
        // The device on the monitor adapter, the duplication and the band staging texture
        bool openOn(IDXGIAdapter1* adapter, IDXGIOutput* output, const RECT& desktop, const RECT& band, Duplication& dup)
        {
            ComRef<IDXGIOutput1> output1;
            D3D11_TEXTURE2D_DESC desc{};

            if (FAILED(D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                    dup.device.put(), nullptr, dup.context.put()))
                || FAILED(output->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void**>(output1.put())))
                || FAILED(output1->DuplicateOutput(dup.device.get(), dup.output.put())))
                return false;
            dup.box.left = static_cast<UINT>(band.left - desktop.left);
            dup.box.top = static_cast<UINT>(band.top - desktop.top);
            dup.box.right = static_cast<UINT>((band.right < desktop.right ? band.right : desktop.right) - desktop.left);
            dup.box.bottom = static_cast<UINT>((band.bottom < desktop.bottom ? band.bottom : desktop.bottom) - desktop.top);
            dup.box.back = 1;
            desc.Width = dup.box.right - dup.box.left;
            desc.Height = dup.box.bottom - dup.box.top;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_STAGING;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            return SUCCEEDED(dup.device->CreateTexture2D(&desc, nullptr, dup.staging.put()));
        }

        /*
        ** Монитор, на котором лежит левый верхний угол полосы: перебор адаптеров и их выходов. Полоса
        ** обрезается по монитору и переводится в его координаты; staging-текстура — её размера.
        **
        ** The monitor holding the band top left corner: going over the adapters and their outputs. The band is
        ** clipped by the monitor and converted to its coordinates; the staging texture has its size.
        */
        bool openDuplication(const RECT& band, Duplication& dup)
        {
            ComRef<IDXGIFactory1> factory;
            ComRef<IDXGIAdapter1> adapter;
            ComRef<IDXGIOutput> output;
            DXGI_OUTPUT_DESC desc{};
            POINT corner{band.left, band.top};

            if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(factory.put()))))
                return false;
            for (UINT index = 0; factory->EnumAdapters1(index, adapter.put()) != DXGI_ERROR_NOT_FOUND; ++index)
                for (UINT number = 0; adapter->EnumOutputs(number, output.put()) != DXGI_ERROR_NOT_FOUND; ++number)
                    if (SUCCEEDED(output->GetDesc(&desc)) && PtInRect(&desc.DesktopCoordinates, corner))
                        return openOn(adapter.get(), output.get(), desc.DesktopCoordinates, band, dup);
            return false;
        }

        /*
        ** Следующий кадр монитора: полоса копируется в staging и считаются плохие столбцы.
        ** -1 — нового кадра за 20 мс не было (рабочий стол не менялся).
        **
        ** The next monitor frame: the band is copied to staging and the bad columns are counted.
        ** -1 — there was no new frame within 20 ms (the desktop did not change).
        */
        int grabFrame(Duplication& dup)
        {
            DXGI_OUTDUPL_FRAME_INFO info{};
            ComRef<IDXGIResource> resource;
            ComRef<ID3D11Texture2D> frame;
            D3D11_MAPPED_SUBRESOURCE mapped{};
            int columns = 0;

            if (FAILED(dup.output->AcquireNextFrame(ACQUIRE_TIMEOUT_MS, &info, resource.put())))
                return -1;
            if (SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(frame.put()))))
                dup.context->CopySubresourceRegion(dup.staging.get(), 0, 0, 0, 0, frame.get(), 0, &dup.box);
            // Кадр возвращается сразу: пока он удержан, DWM не может отдать следующий
            // The frame is returned at once: while it is held DWM cannot give the next one
            dup.output->ReleaseFrame();
            if (SUCCEEDED(dup.context->Map(dup.staging.get(), 0, D3D11_MAP_READ, 0, &mapped)))
            {
                columns = badColumns(static_cast<const std::uint8_t*>(mapped.pData), mapped.RowPitch,
                    static_cast<int>(dup.box.right - dup.box.left), static_cast<int>(dup.box.bottom - dup.box.top));
                dup.context->Unmap(dup.staging.get(), 0);
            }
            return columns;
        }

        // Тело потока наблюдателя (функтор вместо лямбды) / The watcher thread body (a functor instead of a lambda)
        struct WatchLoop
        {
            std::shared_ptr<WatchShared> shared;

            void operator()() const
            {
                Duplication dup;
                WatchResult& result = shared->result;

                result.opened = openDuplication(shared->band, dup);
                while (result.opened && !shared->stop.load())
                {
                    int columns = grabFrame(dup);
                    if (columns < 0)
                        continue;
                    ++result.frames;
                    if (columns >= BAD_FRAME_COLUMNS)
                        ++result.badFrames;
                    if (columns > result.worstColumns)
                        result.worstColumns = columns;
                }
            }
        };
    }

    // Запуск потока наблюдателя с телом WatchLoop / Starting the watcher thread with the WatchLoop body
    void startWatchLoop(const std::shared_ptr<WatchShared>& shared, std::thread& thread)
    {
        thread = std::thread(WatchLoop{shared});
    }
}
