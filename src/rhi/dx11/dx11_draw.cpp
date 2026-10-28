#include "gran/rhi/dx11/internal/dx11_backend_impl.hpp"

namespace gran::rhi::dx11
{
    // Возвращаем цель после Present и проходов размытия.
    // Restore the target after Present and blur passes.
    void Dx11Backend::bindTarget(ID3D11RenderTargetView* target, UINT width, UINT height)
    {
        D3D11_VIEWPORT viewport{0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1};

        _context->OMSetRenderTargets(1, &target, nullptr);
        _context->RSSetViewports(1, &viewport);
    }

    // Фиксированные SM5-регистры: экземпляры, отсечения, атлас, рампа, картинка, подложка.
    // Fixed SM5 registers: instances, clips, atlas, ramp, image, backdrop.
    void Dx11Backend::bindUi(const FrameData& frame)
    {
        ID3D11ShaderResourceView* views[] = {_instances.view.get(), _clips.view.get(),
            _textures[frame.atlasSlot].view.get(), _textures[frame.rampSlot].view.get(), nullptr, _blur[1].view.get()};
        ID3D11SamplerState* sampler = _sampler.get();

        bindTarget(_surface.target.get(), _options.width, _options.height);
        _context->VSSetShader(_vertex.get(), nullptr, 0);
        _context->PSSetShader(_pixel.get(), nullptr, 0);
        _context->VSSetShaderResources(0, 2, views);
        _context->PSSetShaderResources(0, 6, views);
        _context->PSSetSamplers(0, 1, &sampler);
        _context->IASetInputLayout(nullptr);
        _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        _context->RSSetState(_raster.get());
        _context->OMSetDepthStencilState(_depth.get(), 0);
        _context->OMSetBlendState(_blend.get(), nullptr, 0xffffffff);
    }

    // Один пакет с общей картинкой; константы содержат абсолютный индекс экземпляра.
    // One batch sharing an image; constants contain the absolute instance index.
    void Dx11Backend::drawBatch(const FrameData& frame, UINT first, UINT count, UINT image)
    {
        render::FrameConstants data{};
        ID3D11ShaderResourceView* view = nullptr;

        data.viewport[0] = static_cast<float>(_options.width);
        data.viewport[1] = static_cast<float>(_options.height);
        data.inverseViewport[0] = 1.0f / data.viewport[0];
        data.inverseViewport[1] = 1.0f / data.viewport[1];
        data.instanceBase = first;
        data.clipCount = frame.clipCount;
        data.textureCount = static_cast<UINT>(_textures.size());
        data.rampRows = frame.rampRows;
        data.atlasSlot = frame.atlasSlot;
        data.rampSlot = frame.rampSlot;
        if (image < _textures.size())
            view = _textures[image].view.get();
        _context->PSSetShaderResources(4, 1, &view);
        constants(data);
        _context->DrawInstanced(4, count, 0, 0);
    }

    // Разбиваем только при смене картинки, сохраняя порядок прозрачных экземпляров.
    // Split only on image changes, preserving transparent instance order.
    void Dx11Backend::drawSegment(const FrameData& frame, const FrameSegment& segment)
    {
        UINT first = segment.first;
        UINT end = first + segment.count;
        UINT image = MAX_TEXTURES;

        for (UINT index = first; index < end; ++index)
        {
            const render::GpuInstance& item = frame.instances[index];
            if (item.kind != render::KIND_IMAGE || item.paint[0] == image)
                continue;
            if (image != MAX_TEXTURES)
            {
                drawBatch(frame, first, index - first, image);
                first = index;
            }
            image = item.paint[0];
        }
        if (first < end)
            drawBatch(frame, first, end - first, image);
    }

    // Подложка снимается перед сегментом, затем возвращаются привязки UI.
    // Capture the backdrop before its segment, then restore UI bindings.
    void Dx11Backend::drawFrame(const FrameData& frame)
    {
        upload(_instances, frame.instances, frame.instanceCount, sizeof(render::GpuInstance));
        upload(_clips, frame.clips, frame.clipCount, sizeof(render::GpuClip));
        bindUi(frame);
        _context->ClearRenderTargetView(_surface.target.get(), frame.clearColor);
        for (UINT index = 0; index < frame.segmentCount; ++index)
        {
            const FrameSegment& segment = frame.segments[index];
            if (segment.backdropBefore)
            {
                backdrop(segment.backdropRadius);
                bindUi(frame);
            }
            drawSegment(frame, segment);
        }
    }
}
