#include "gran/rhi/dx11/internal/dx11_backend_impl.hpp"
#include "gran/rhi/shader_blobs.hpp"

namespace gran::rhi::dx11
{
    // Только встроенный DXBC: компилятор шейдеров не нужен в программе заказчика.
    // Embedded DXBC only: the customer's program needs no shader compiler.
    void Dx11Backend::createPipeline()
    {
        using namespace generated;
        D3D11_BUFFER_DESC desc{};
        D3D11_QUERY_DESC query{D3D11_QUERY_EVENT, 0};

        GRAN_DX11_CHECK(_device->CreateVertexShader(UI_VS_DXBC, UI_VS_DXBC_SIZE, nullptr, _vertex.put()));
        GRAN_DX11_CHECK(_device->CreateVertexShader(FULLSCREEN_VS_DXBC, FULLSCREEN_VS_DXBC_SIZE, nullptr, _fullscreen.put()));
        GRAN_DX11_CHECK(_device->CreatePixelShader(UI_PS_DXBC, UI_PS_DXBC_SIZE, nullptr, _pixel.put()));
        GRAN_DX11_CHECK(_device->CreatePixelShader(BLUR_DOWN_PS_DXBC, BLUR_DOWN_PS_DXBC_SIZE, nullptr, _down.put()));
        GRAN_DX11_CHECK(_device->CreatePixelShader(BLUR_UP_PS_DXBC, BLUR_UP_PS_DXBC_SIZE, nullptr, _up.put()));
        desc.ByteWidth = sizeof(render::FrameConstants);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        GRAN_DX11_CHECK(_device->CreateBuffer(&desc, nullptr, _constants.put()));
        GRAN_DX11_CHECK(_device->CreateQuery(&query, _idle.put()));
        nameObject(_constants.get(), "gran.dx11.constants");
        nameObject(_vertex.get(), "gran.dx11.ui.vs");
        nameObject(_pixel.get(), "gran.dx11.ui.ps");
        nameObject(_fullscreen.get(), "gran.dx11.fullscreen.vs");
        nameObject(_down.get(), "gran.dx11.blur.down");
        nameObject(_up.get(), "gran.dx11.blur.up");
        nameObject(_idle.get(), "gran.dx11.idle");
        createStates();
    }

    // Предумноженная альфа, линейная выборка, без глубины и отсечения граней.
    // Premultiplied alpha, linear sampling, no depth and no face culling.
    void Dx11Backend::createStates()
    {
        D3D11_SAMPLER_DESC sampler{};
        D3D11_BLEND_DESC blend{};
        D3D11_RASTERIZER_DESC raster{};
        D3D11_DEPTH_STENCIL_DESC depth{};

        sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampler.MaxLOD = D3D11_FLOAT32_MAX;
        blend.RenderTarget[0].BlendEnable = TRUE;
        blend.RenderTarget[0].SrcBlend = blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blend.RenderTarget[0].DestBlend = blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        blend.RenderTarget[0].BlendOp = blend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        raster.FillMode = D3D11_FILL_SOLID;
        raster.CullMode = D3D11_CULL_NONE;
        raster.DepthClipEnable = TRUE;
        GRAN_DX11_CHECK(_device->CreateSamplerState(&sampler, _sampler.put()));
        GRAN_DX11_CHECK(_device->CreateBlendState(&blend, _blend.put()));
        GRAN_DX11_CHECK(_device->CreateRasterizerState(&raster, _raster.put()));
        GRAN_DX11_CHECK(_device->CreateDepthStencilState(&depth, _depth.put()));
        nameObject(_sampler.get(), "gran.dx11.sampler");
        nameObject(_blend.get(), "gran.dx11.blend");
        nameObject(_raster.get(), "gran.dx11.raster");
        nameObject(_depth.get(), "gran.dx11.depth");
    }
}
