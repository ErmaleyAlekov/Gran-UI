#include "gran/rhi/metal/internal/metal_backend_impl.hpp"
#include "gran/rhi/shader_blobs.hpp"

namespace gran::rhi::metal
{
    // Системное устройство и очередь; программного адаптера (как WARP или lavapipe) у Metal нет — forceWarp
    // не поддерживается. Отладочный слой Metal включается переменными окружения процесса (MTL_DEBUG_LAYER=1,
    // MTL_SHADER_VALIDATION=1) до первого обращения к Metal — из кода его не включить.
    // The system device and queue; Metal has no software adapter (like WARP or lavapipe) — forceWarp
    // is not supported. The Metal debug layer is enabled by the process environment variables (MTL_DEBUG_LAYER=1,
    // MTL_SHADER_VALIDATION=1) before the first Metal call — it cannot be enabled from code.
    void MetalBackend::createDevice()
    {
        if (_options.forceWarp)
            throw GpuException(ErrorCode::GPU, i18n::format(i18n::Message::GPU_NO_ADAPTER, {"Metal software"}), 0);
        _device = MTLCreateSystemDefaultDevice();
        if (_device == nil)
            throw GpuException(ErrorCode::GPU, i18n::format(i18n::Message::GPU_NO_ADAPTER, {"Metal"}), 0);
        _queue = [_device newCommandQueue];
        GRAN_METAL_CHECK(_queue != nil, nil, "newCommandQueue");
        _queue.label = @"gran.metal.queue";
    }

    // Библиотека шейдеров собрана при сборке (xcrun metal → metallib → gran_embed): в движке шейдеры
    // не компилируются. Сэмплер — линейный с прижатием к краю, как в D3D11 и Vulkan.
    // The shader library is built at build time (xcrun metal → metallib → gran_embed): shaders are not
    // compiled in the engine. The sampler is linear with clamping to the edge, as in D3D11 and Vulkan.
    void MetalBackend::createPipelines()
    {
        NSError* error = nil;
        dispatch_data_t data = dispatch_data_create(generated::METAL_LIBRARY, generated::METAL_LIBRARY_SIZE,
            nil, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
        _library = [_device newLibraryWithData:data error:&error];
        GRAN_METAL_CHECK(_library != nil, error, "newLibraryWithData");
        _library.label = @"gran.metal.library";
        _ui = makePipeline(@"vsMain", @"psMain", true, "gran.metal.ui");
        _down = makePipeline(@"vsFullscreen", @"psBlurDown", false, "gran.metal.blur.down");
        _up = makePipeline(@"vsFullscreen", @"psBlurUp", false, "gran.metal.blur.up");
        MTLSamplerDescriptor* sampler = [[MTLSamplerDescriptor alloc] init];
        sampler.minFilter = MTLSamplerMinMagFilterLinear;
        sampler.magFilter = MTLSamplerMinMagFilterLinear;
        sampler.sAddressMode = MTLSamplerAddressModeClampToEdge;
        sampler.tAddressMode = MTLSamplerAddressModeClampToEdge;
        sampler.label = @"gran.metal.sampler";
        _sampler = [_device newSamplerStateWithDescriptor:sampler];
        GRAN_METAL_CHECK(_sampler != nil, nil, "newSamplerStateWithDescriptor");
    }

    // Конвейер: вершинная и пиксельная функции из библиотеки, цель RGBA8. blend — смешивание с предумноженной
    // альфой (ONE, ONE_MINUS_SRC_ALPHA), как в D3D11; проходы размытия пишут без смешивания.
    // A pipeline: the vertex and fragment functions from the library, an RGBA8 target. blend — blending with
    // premultiplied alpha (ONE, ONE_MINUS_SRC_ALPHA), as in D3D11; the blur passes write without blending.
    id<MTLRenderPipelineState> MetalBackend::makePipeline(NSString* vertex, NSString* fragment, bool blend, const char* name)
    {
        NSError* error = nil;
        MTLRenderPipelineDescriptor* info = [[MTLRenderPipelineDescriptor alloc] init];
        info.vertexFunction = [_library newFunctionWithName:vertex];
        info.fragmentFunction = [_library newFunctionWithName:fragment];
        GRAN_METAL_CHECK(info.vertexFunction != nil && info.fragmentFunction != nil, nil, "newFunctionWithName");
        info.label = [NSString stringWithUTF8String:name];
        MTLRenderPipelineColorAttachmentDescriptor* color = info.colorAttachments[0];
        color.pixelFormat = MTLPixelFormatRGBA8Unorm;
        color.blendingEnabled = blend;
        color.sourceRGBBlendFactor = MTLBlendFactorOne;
        color.sourceAlphaBlendFactor = MTLBlendFactorOne;
        color.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        color.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        id<MTLRenderPipelineState> pipeline = [_device newRenderPipelineStateWithDescriptor:info error:&error];
        GRAN_METAL_CHECK(pipeline != nil, error, "newRenderPipelineStateWithDescriptor");
        return pipeline;
    }
}
