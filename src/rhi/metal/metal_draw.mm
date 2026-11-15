#include "gran/rhi/metal/internal/metal_backend_impl.hpp"

namespace gran::rhi::metal
{
    namespace
    {
        // Проход в поверхность кадра: clear — очистить цветом фона, иначе загрузить уже нарисованное
        // A pass into the frame surface: clear — clear with the background color, otherwise load what is already drawn
        MTLRenderPassDescriptor* surfacePass(id<MTLTexture> target, const FrameData& frame, bool clear)
        {
            MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];

            pass.colorAttachments[0].texture = target;
            pass.colorAttachments[0].storeAction = MTLStoreActionStore;
            pass.colorAttachments[0].loadAction = MTLLoadActionLoad;
            if (clear)
                pass.colorAttachments[0].loadAction = MTLLoadActionClear;
            pass.colorAttachments[0].clearColor = MTLClearColorMake(frame.clearColor[0], frame.clearColor[1],
                frame.clearColor[2], frame.clearColor[3]);
            return pass;
        }
    }

    // Проход интерфейса по сегментам; подложка требует копии кадра — проход прерывается и начинается снова
    // с загрузкой уже нарисованного (loadAction Load)
    // The UI pass over the segments; a backdrop needs a frame copy — the pass is interrupted and started again
    // loading what is already drawn (loadAction Load)
    void MetalBackend::drawFrame(id<MTLCommandBuffer> buffer, const FrameData& frame)
    {
        id<MTLRenderCommandEncoder> encoder = beginUi(buffer, frame, true);

        for (std::uint32_t index = 0; index < frame.segmentCount; ++index)
        {
            const FrameSegment& segment = frame.segments[index];
            if (segment.backdropBefore)
            {
                [encoder endEncoding];
                backdrop(buffer, segment.backdropRadius);
                encoder = beginUi(buffer, frame, false);
            }
            drawSegment(encoder, frame, segment);
        }
        [encoder endEncoding];
    }

    // Привязки — по номерам из ветки GRAN_METAL в ui_common.hlsli: buffer(1) экземпляры, buffer(2) отсечения,
    // texture(0) атлас, (1) градиенты, (2) картинка — по пакету, (3) подложка; sampler(0)
    // The bindings follow the GRAN_METAL branch numbers in ui_common.hlsli: buffer(1) the instances, buffer(2) the clips,
    // texture(0) the atlas, (1) the ramps, (2) the image — per batch, (3) the backdrop; sampler(0)
    id<MTLRenderCommandEncoder> MetalBackend::beginUi(id<MTLCommandBuffer> buffer, const FrameData& frame, bool clear)
    {
        const FrameBuffers& data = _frames[_frameIndex];
        id<MTLTexture> backdropTexture = _dummy.resource;
        id<MTLRenderCommandEncoder> encoder = [buffer renderCommandEncoderWithDescriptor:surfacePass(_surface.resource,
            frame, clear)];
        GRAN_METAL_CHECK(encoder != nil, nil, "renderCommandEncoderWithDescriptor");
        encoder.label = @"gran.metal.ui";
        if (_blur[1].resource != nil)
            backdropTexture = _blur[1].resource;
        [encoder setRenderPipelineState:_ui];
        [encoder setViewport:MTLViewport{0, 0, static_cast<double>(_options.width), static_cast<double>(_options.height), 0, 1}];
        [encoder setVertexBuffer:data.instances offset:0 atIndex:1];
        [encoder setFragmentBuffer:data.instances offset:0 atIndex:1];
        [encoder setFragmentBuffer:data.clips offset:0 atIndex:2];
        [encoder setFragmentTexture:_textures[frame.atlasSlot].resource atIndex:0];
        [encoder setFragmentTexture:_textures[frame.rampSlot].resource atIndex:1];
        [encoder setFragmentTexture:backdropTexture atIndex:3];
        [encoder setFragmentSamplerState:_sampler atIndex:0];
        return encoder;
    }

    // Пакеты внутри сегмента: картинка у пакета одна, поэтому смена картинки начинает новый пакет (как в D3D11)
    // The batches inside a segment: a batch has one image, so an image change starts a new batch (as in D3D11)
    void MetalBackend::drawSegment(id<MTLRenderCommandEncoder> encoder, const FrameData& frame, const FrameSegment& segment)
    {
        std::uint32_t first = segment.first;
        std::uint32_t end = first + segment.count;
        std::uint32_t image = MAX_TEXTURES;

        for (std::uint32_t index = first; index < end; ++index)
        {
            const render::GpuInstance& item = frame.instances[index];
            if (item.kind != render::KIND_IMAGE || item.paint[0] == image)
                continue;
            if (image != MAX_TEXTURES)
            {
                drawBatch(encoder, frame, first, index - first, image);
                first = index;
            }
            image = item.paint[0];
        }
        if (first < end)
            drawBatch(encoder, frame, first, end - first, image);
    }

    // Пакет: константы кадра копируются в командный буфер (setVertexBytes), 4 вершины полосы на экземпляр.
    // Базовый экземпляр 0: instance_id в Metal включает его, а шейдер сам прибавляет frame.instanceBase.
    // A batch: the frame constants are copied into the command buffer (setVertexBytes), 4 strip vertices per instance.
    // Base instance 0: instance_id in Metal includes it, and the shader adds frame.instanceBase itself.
    void MetalBackend::drawBatch(id<MTLRenderCommandEncoder> encoder, const FrameData& frame, std::uint32_t first,
        std::uint32_t count, std::uint32_t image)
    {
        render::FrameConstants data{};
        id<MTLTexture> view = _dummy.resource;

        data.viewport[0] = static_cast<float>(_options.width);
        data.viewport[1] = static_cast<float>(_options.height);
        data.inverseViewport[0] = 1.0f / data.viewport[0];
        data.inverseViewport[1] = 1.0f / data.viewport[1];
        data.instanceBase = first;
        data.clipCount = frame.clipCount;
        data.textureCount = static_cast<std::uint32_t>(_textures.size());
        data.rampRows = frame.rampRows;
        data.atlasSlot = frame.atlasSlot;
        data.rampSlot = frame.rampSlot;
        if (image < _textures.size())
            view = _textures[image].resource;
        [encoder setFragmentTexture:view atIndex:2];
        [encoder setVertexBytes:&data length:sizeof(data) atIndex:0];
        [encoder setFragmentBytes:&data length:sizeof(data) atIndex:0];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4 instanceCount:count];
    }
}
