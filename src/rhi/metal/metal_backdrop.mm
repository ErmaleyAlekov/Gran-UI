#include "gran/rhi/metal/internal/metal_backend_impl.hpp"

#include <algorithm>
#include <cmath>

namespace gran::rhi::metal
{
    // Подложка: копия кадра в уровень 0, затем двойной Kawase — вниз до passes и обратно вверх до уровня 1.
    // Число проходов и порядок — как в D3D11, иначе размытие разошлось бы с эталонами.
    // The backdrop: a frame copy into level 0, then dual Kawase — down to passes and back up to level 1.
    // The pass count and the order are as in D3D11, otherwise the blur would diverge from the baselines.
    void MetalBackend::backdrop(id<MTLCommandBuffer> buffer, float radius)
    {
        auto passes = static_cast<std::uint32_t>(std::lround(std::log2(std::max(2.0f, radius / 1.5f))));

        passes = std::min(BLUR_LEVELS - 1, std::max(1u, passes));
        ensureBackdrop();
        id<MTLBlitCommandEncoder> blit = [buffer blitCommandEncoder];
        GRAN_METAL_CHECK(blit != nil, nil, "blitCommandEncoder");
        blit.label = @"gran.metal.backdrop.copy";
        [blit copyFromTexture:_surface.resource toTexture:_blur[0].resource];
        [blit endEncoding];
        for (std::uint32_t level = 1; level <= passes; ++level)
            blur(buffer, level - 1, level, _down);
        for (std::uint32_t level = passes - 1; level >= 1; --level)
            blur(buffer, level + 1, level, _up);
    }

    // Уровни размытия создаются при первой подложке: каждый вдвое меньше предыдущего, уровень 0 — размер кадра
    // The blur levels are created at the first backdrop: each is half the previous one, level 0 is the frame size
    void MetalBackend::ensureBackdrop()
    {
        if (_blur[0].resource != nil)
            return;
        for (std::uint32_t level = 0; level < BLUR_LEVELS; ++level)
            makeTexture(_device, _blur[level], TextureDesc{std::max(1u, _options.width >> level),
                std::max(1u, _options.height >> level), TextureFormat::RGBA8, "gran.metal.blur"}, true);
    }

    // Один проход: полноэкранный треугольник читает уровень source (texture(2)) и пишет весь уровень destination.
    // viewport — размер цели, inverseViewport — шаг текселя источника (как в D3D11).
    // One pass: a fullscreen triangle reads the source level (texture(2)) and writes the whole destination level.
    // viewport is the target size, inverseViewport is the source texel step (as in D3D11).
    void MetalBackend::blur(id<MTLCommandBuffer> buffer, std::uint32_t source, std::uint32_t destination,
        id<MTLRenderPipelineState> pipeline)
    {
        MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
        const Texture& target = _blur[destination];
        render::FrameConstants data{};

        pass.colorAttachments[0].texture = target.resource;
        pass.colorAttachments[0].loadAction = MTLLoadActionDontCare;
        pass.colorAttachments[0].storeAction = MTLStoreActionStore;
        data.viewport[0] = static_cast<float>(target.width);
        data.viewport[1] = static_cast<float>(target.height);
        data.inverseViewport[0] = 1.0f / static_cast<float>(_blur[source].width);
        data.inverseViewport[1] = 1.0f / static_cast<float>(_blur[source].height);
        id<MTLRenderCommandEncoder> encoder = [buffer renderCommandEncoderWithDescriptor:pass];
        GRAN_METAL_CHECK(encoder != nil, nil, "renderCommandEncoderWithDescriptor");
        encoder.label = @"gran.metal.blur";
        [encoder setRenderPipelineState:pipeline];
        [encoder setViewport:MTLViewport{0, 0, static_cast<double>(target.width), static_cast<double>(target.height), 0, 1}];
        [encoder setFragmentTexture:_blur[source].resource atIndex:2];
        [encoder setFragmentSamplerState:_sampler atIndex:0];
        [encoder setVertexBytes:&data length:sizeof(data) atIndex:0];
        [encoder setFragmentBytes:&data length:sizeof(data) atIndex:0];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
        [encoder endEncoding];
    }
}
