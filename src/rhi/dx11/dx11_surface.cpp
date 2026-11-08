#include "gran/rhi/dx11/internal/dx11_backend_impl.hpp"

namespace gran::rhi::dx11
{
    // Унифицированная текстура R8/RGBA8, при необходимости с RTV.
    // A unified R8/RGBA8 texture with an optional RTV.
    void makeTexture(ID3D11Device* device, Texture& texture, const TextureDesc& desc, bool target)
    {
        D3D11_TEXTURE2D_DESC resource{};

        if (desc.width == 0 || desc.height == 0 || desc.width > 16384 || desc.height > 16384)
            invalid("size");
        resource.Width = desc.width;
        resource.Height = desc.height;
        resource.MipLevels = resource.ArraySize = resource.SampleDesc.Count = 1;
        resource.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        if (desc.format == TextureFormat::R8)
            resource.Format = DXGI_FORMAT_R8_UNORM;
        resource.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        if (target)
            resource.BindFlags |= D3D11_BIND_RENDER_TARGET;
        GRAN_DX11_CHECK(device->CreateTexture2D(&resource, nullptr, texture.resource.put()));
        GRAN_DX11_CHECK(device->CreateShaderResourceView(texture.resource.get(), nullptr, texture.view.put()));
        if (target)
            GRAN_DX11_CHECK(device->CreateRenderTargetView(texture.resource.get(), nullptr, texture.target.put()));
        texture.width = desc.width;
        texture.height = desc.height;
        texture.bytes = 4;
        if (desc.format == TextureFormat::R8)
            texture.bytes = 1;
        nameObject(texture.resource.get(), desc.debugName);
        nameObject(texture.view.get(), desc.debugName);
        nameObject(texture.target.get(), desc.debugName);
    }

    // Flip-sequential swapchain совместим с D3D11 и не создаёт отдельную копию кадра.
    // A flip-sequential swapchain supports D3D11 without a separate frame copy.
    void Dx11Backend::createSurface()
    {
        DXGI_SWAP_CHAIN_DESC1 desc{};

        if (_options.nativeWindow == nullptr)
        {
            makeTexture(_device.get(), _surface, TextureDesc{_options.width, _options.height,
                TextureFormat::RGBA8, "gran.dx11.offscreen"}, true);
            return;
        }
        desc.Width = _options.width;
        desc.Height = _options.height;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        if (!_swapchain)
            GRAN_DX11_CHECK(dxgi::createWindowChain(dxgi::WindowChainRequest{_factory.get(), _device.get(),
                static_cast<HWND>(_options.nativeWindow), desc}, _composition, _swapchain.put()));
        GRAN_DX11_CHECK(_factory->MakeWindowAssociation(static_cast<HWND>(_options.nativeWindow), DXGI_MWA_NO_ALT_ENTER));
        GRAN_DX11_CHECK(_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(_surface.resource.put())));
        GRAN_DX11_CHECK(_device->CreateRenderTargetView(_surface.resource.get(), nullptr, _surface.target.put()));
        _surface.width = _options.width;
        _surface.height = _options.height;
        nameObject(_surface.resource.get(), "gran.dx11.swapchain");
        nameObject(_surface.target.get(), "gran.dx11.target");
    }
}
