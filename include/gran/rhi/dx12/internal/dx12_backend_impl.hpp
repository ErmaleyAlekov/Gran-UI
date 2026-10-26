#ifndef GRAN_RHI_DX12_INTERNAL_DX12_BACKEND_IMPL_HPP
#define GRAN_RHI_DX12_INTERNAL_DX12_BACKEND_IMPL_HPP

#include "gran/rhi/dx12/internal/dx12_internal.hpp"

#include <memory>

namespace gran::rhi::dx12
{
    // Ресурсы одного кадра в полёте / The resources of one frame in flight
    struct FrameContext
    {
        ComRef<ID3D12CommandAllocator> allocator;
        UploadRing ring;
        std::uint64_t fenceValue = 0;
    };

    // Адреса данных кадра в буфере загрузки / The frame data addresses in the upload buffer
    struct FrameAddresses
    {
        D3D12_GPU_VIRTUAL_ADDRESS instances = 0;
        D3D12_GPU_VIRTUAL_ADDRESS clips = 0;
    };

    /*
    ** Бэкенд D3D12. Поля объявлены в порядке создания и уничтожаются в обратном:
    ** поверхность и список команд — первыми, устройство и фабрика — последними.
    ** Деструктор сначала ждёт простоя GPU.
    **
    ** The D3D12 backend. The fields are declared in the creation order and destroyed in reverse:
    ** the surface and the command list first, the device and the factory last.
    ** The destructor first waits for the GPU to become idle.
    */
    class Dx12Backend final : public RenderBackend, public SnapshotSource
    {
    public:
        // ---- dx12_backend.cpp ----
        explicit Dx12Backend(const BackendOptions& options);
        ~Dx12Backend() override;
        void resize(std::uint32_t width, std::uint32_t height) override;

        // ---- dx12_textures.cpp ----
        std::uint32_t createTexture(const TextureDesc& desc) override;
        void updateTexture(std::uint32_t slot, const TextureRegion& region, const std::uint8_t* pixels,
            std::uint32_t rowPitch) override;
        void releaseTexture(std::uint32_t slot) override;
        void readPixels(std::vector<std::uint8_t>& rgba, std::uint32_t& width, std::uint32_t& height) override;
        std::vector<std::string> takeDiagnostics() override;

        // ---- dx12_frame.cpp ----
        FrameStatus render(const FrameData& frame) override;

    private:
        // ---- dx12_backend.cpp ----
        // Отправить сигнал и дождаться GPU / Send a signal and wait for the GPU
        void waitIdle();

        // ---- dx12_frame.cpp ----
        // Проверка данных кадра до записи на GPU / Checking the frame data before writing to the GPU
        void validate(const FrameData& frame) const;

        // Ожидание кадра, освобождение пройденного, сброс списка и кольца / Waiting for the frame, releasing the passed, resetting the list and the ring
        FrameContext& beginFrame(const FrameData& frame);

        // Отправка, показ и сигнал забора / Submitting, presenting and the fence signal
        FrameStatus submit(FrameContext& context);

        // ---- dx12_record.cpp ----
        // Копия экземпляров и отсечений в кольцо / Copying the instances and clips into the ring
        FrameAddresses uploadFrame(const FrameData& frame, FrameContext& context);

        // Запись барьеров, очистки и вызовов отрисовки / Recording the barriers, the clear and the draw calls
        void recordDraws(const FrameData& frame, const FrameAddresses& addresses);

        // ---- dx12_backdrop_chain.cpp ----
        // Цепочка размытия под текущий размер цели / The blur chain for the current target size
        void ensureBackdrop();

        // ---- dx12_backdrop.cpp ----
        // Размытие копии кадра для сегмента с подложкой / Blurring a frame copy for a segment with a backdrop
        void recordBackdrop(float radius);

        // Один проход размытия из уровня в уровень / One blur pass from level to level
        void blurPass(std::uint32_t source, std::uint32_t destination, ID3D12PipelineState* pipeline);

        DeviceContext _context;
        FenceSync _fence;
        PipelineSet _pipelines;
        TextureTable _textures;
        DeferredReleases _releases;
        FrameContext _frames[FRAMES_IN_FLIGHT];
        ComRef<ID3D12GraphicsCommandList> _list;
        BackdropChain _backdrop;
        std::unique_ptr<Surface> _surface;
        BackendOptions _options;
        std::uint32_t _frameIndex = 0;
        std::uint64_t _lastSignaled = 0;
        bool _offscreen = false;
        std::vector<std::string> _diagnostics;
    };
}

#endif
