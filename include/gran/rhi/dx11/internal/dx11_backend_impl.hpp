#ifndef GRAN_RHI_DX11_INTERNAL_BACKEND_IMPL_HPP
#define GRAN_RHI_DX11_INTERNAL_BACKEND_IMPL_HPP

#include "gran/rhi/dx11/dx11_backend.hpp"
#include "gran/rhi/dx12/internal/com_ref.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d11.h>
#include <d3d11sdklayers.h>
#include <dxgi1_2.h>

namespace gran::rhi::dx11
{
    using dx12::ComRef;
    constexpr UINT BLUR_LEVELS = 6;
    constexpr UINT MAX_TEXTURES = 4096;

    // Владеющие ссылки: D3D11 удерживает используемые команды до завершения GPU.
    // Owning references: D3D11 retains submitted resources until GPU work completes.
    struct Texture
    {
        ComRef<ID3D11Texture2D> resource;
        ComRef<ID3D11ShaderResourceView> view;
        ComRef<ID3D11RenderTargetView> target;
        UINT width = 0;
        UINT height = 0;
        UINT bytes = 4;
    };

    // Буфер DEFAULT: UpdateSubresource оставляет синхронизацию и версии данных драйверу.
    // A DEFAULT buffer: UpdateSubresource leaves synchronization and data versions to the driver.
    struct StructuredBuffer
    {
        ComRef<ID3D11Buffer> resource;
        ComRef<ID3D11ShaderResourceView> view;
        UINT capacity = 0;
    };

    // Проверка HRESULT и именование объектов через каталог ошибок.
    // HRESULT checking and object naming through the error catalog.
    void check(HRESULT result, const char* call, const char* file, int line);
    bool deviceLost(HRESULT result) noexcept;
    void nameObject(ID3D11DeviceChild* object, const char* name);
    void invalid(const char* resource);
    void makeTexture(ID3D11Device* device, Texture& texture, const TextureDesc& desc, bool target);

    /*
    ** Порядок полей задаёт обратное разрушение: ресурсы → контекст → устройство → фабрика.
    ** Перед разрушением и resize ждём EVENT query; обычный кадр не ждёт простоя GPU.
    ** D3D11 сам удерживает ресурсы отправленных команд и переименовывает Map(DISCARD).
    **
    ** Fields are destroyed in reverse: resources → context → device → factory.
    ** Destruction and resize wait for an EVENT query; normal frames never wait for GPU idle.
    ** D3D11 retains submitted resources and renames Map(DISCARD) allocations itself.
    */
    class Dx11Backend final : public RenderBackend, public SnapshotSource
    {
    public:
        explicit Dx11Backend(const BackendOptions& options);
        ~Dx11Backend() override;
        void resize(std::uint32_t width, std::uint32_t height) override;
        std::uint32_t createTexture(const TextureDesc& desc) override;
        void updateTexture(std::uint32_t slot, const TextureRegion& region,
            const std::uint8_t* pixels, std::uint32_t rowPitch) override;
        void releaseTexture(std::uint32_t slot) override;
        FrameStatus render(const FrameData& frame) override;
        std::vector<std::string> takeDiagnostics() override;
        void readPixels(std::vector<std::uint8_t>& rgba, std::uint32_t& width, std::uint32_t& height) override;

    private:
        void createDevice();
        HRESULT tryDevice(D3D_DRIVER_TYPE type, UINT flags);
        void createPipeline();
        void createStates();
        void createSurface();
        void waitIdle();
        void upload(StructuredBuffer& buffer, const void* data, UINT count, UINT stride);
        void validate(const FrameData& frame) const;
        void drawFrame(const FrameData& frame);
        void bindTarget(ID3D11RenderTargetView* target, UINT width, UINT height);
        void bindUi(const FrameData& frame);
        void constants(const render::FrameConstants& data);
        void drawSegment(const FrameData& frame, const FrameSegment& segment);
        void drawBatch(const FrameData& frame, UINT first, UINT count, UINT image);
        void backdrop(float radius);
        void ensureBackdrop();
        void blur(UINT source, UINT destination, ID3D11PixelShader* shader);

        ComRef<IDXGIFactory2> _factory;
        ComRef<ID3D11Device> _device;
        ComRef<ID3D11DeviceContext> _context;
        ComRef<ID3D11InfoQueue> _info;
        ComRef<IDXGISwapChain1> _swapchain;
        Texture _surface;
        Texture _blur[BLUR_LEVELS];
        std::vector<Texture> _textures;
        StructuredBuffer _instances;
        StructuredBuffer _clips;
        ComRef<ID3D11Buffer> _constants;
        ComRef<ID3D11VertexShader> _vertex;
        ComRef<ID3D11VertexShader> _fullscreen;
        ComRef<ID3D11PixelShader> _pixel;
        ComRef<ID3D11PixelShader> _down;
        ComRef<ID3D11PixelShader> _up;
        ComRef<ID3D11SamplerState> _sampler;
        ComRef<ID3D11BlendState> _blend;
        ComRef<ID3D11RasterizerState> _raster;
        ComRef<ID3D11DepthStencilState> _depth;
        ComRef<ID3D11Query> _idle;
        BackendOptions _options;
    };
}

#define GRAN_DX11_CHECK(call) ::gran::rhi::dx11::check((call), #call, __FILE__, __LINE__)
#endif
