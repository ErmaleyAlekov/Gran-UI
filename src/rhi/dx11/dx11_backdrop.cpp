#include "gran/rhi/dx11/internal/dx11_backend_impl.hpp"

#include <algorithm>
#include <cmath>

namespace gran::rhi::dx11
{
    // Цепочка создаётся лениво; resize освобождает её после ожидания GPU.
    // The chain is created lazily; resize releases it after waiting for the GPU.
    void Dx11Backend::ensureBackdrop()
    {
        if (_blur[0].resource)
            return;
        for (UINT level = 0; level < BLUR_LEVELS; ++level)
            makeTexture(_device.get(), _blur[level], TextureDesc{std::max(1u, _options.width >> level),
                std::max(1u, _options.height >> level), TextureFormat::RGBA8, "gran.dx11.blur"}, true);
    }

    // Снимаем SRV и RTV до CopyResource: один ресурс не читается и не пишется одновременно.
    // Unbind SRV and RTV before CopyResource: a resource is never read and written simultaneously.
    void Dx11Backend::backdrop(float radius)
    {
        UINT passes = static_cast<UINT>(std::lround(std::log2(std::max(2.0f, radius / 1.5f))));
        ID3D11ShaderResourceView* empty[4] = {};

        passes = std::min(BLUR_LEVELS - 1, std::max(1u, passes));
        ensureBackdrop();
        _context->PSSetShaderResources(2, 4, empty);
        _context->OMSetRenderTargets(0, nullptr, nullptr);
        _context->CopyResource(_blur[0].resource.get(), _surface.resource.get());
        _context->VSSetShader(_fullscreen.get(), nullptr, 0);
        _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        _context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
        for (UINT level = 1; level <= passes; ++level)
            blur(level - 1, level, _down.get());
        for (UINT level = passes - 1; level >= 1; --level)
            blur(level + 1, level, _up.get());
        _context->PSSetShaderResources(4, 1, empty);
        _context->OMSetRenderTargets(0, nullptr, nullptr);
    }

    // Те же Kawase-фильтры и размеры, что в D3D12, с явным снятием конфликтующих SRV.
    // The same Kawase filters and sizes as D3D12, with explicit conflicting SRV unbinding.
    void Dx11Backend::blur(UINT source, UINT destination, ID3D11PixelShader* shader)
    {
        render::FrameConstants data{};
        ID3D11ShaderResourceView* empty = nullptr;
        ID3D11ShaderResourceView* view = _blur[source].view.get();

        _context->PSSetShaderResources(4, 1, &empty);
        bindTarget(_blur[destination].target.get(), _blur[destination].width, _blur[destination].height);
        _context->PSSetShaderResources(4, 1, &view);
        _context->PSSetShader(shader, nullptr, 0);
        data.viewport[0] = static_cast<float>(_blur[destination].width);
        data.viewport[1] = static_cast<float>(_blur[destination].height);
        data.inverseViewport[0] = 1.0f / static_cast<float>(_blur[source].width);
        data.inverseViewport[1] = 1.0f / static_cast<float>(_blur[source].height);
        constants(data);
        _context->Draw(3, 0);
    }
}
