#include "gran/rhi/metal/internal/metal_backend_impl.hpp"

#include <algorithm>
#include <utility>

namespace gran::rhi::metal
{
    /*
    ** Слой окна Cocoa: тот же GPU, формат BGRA8 (из поддержанных CAMetalLayer он ближе всего к RGBA8 поверхности
    ** вне экрана), framebufferOnly = NO — в drawable копирует blit, а не проход отрисовки. Цвета кадра уже в sRGB
    ** (как в браузерах, gpu_rules.md), поэтому пространство слоя — sRGB: macOS сама переведёт их в цвета экрана.
    ** vsync — displaySyncEnabled: без него кадр показывается сразу, с разрывом.
    **
    ** The Cocoa window layer: the same GPU, the BGRA8 format (of those CAMetalLayer supports it is the closest to the
    ** RGBA8 offscreen surface), framebufferOnly = NO — a blit copies into the drawable, not a render pass. The frame
    ** colors are already sRGB (as in browsers, gpu_rules.md), so the layer space is sRGB: macOS itself turns them
    ** into the screen colors. vsync is displaySyncEnabled: without it the frame is shown at once, with tearing.
    */
    void MetalBackend::attachLayer()
    {
        id surface = (__bridge id)_options.nativeWindow;

        if (![surface isKindOfClass:[CAMetalLayer class]])
            throw GpuException(ErrorCode::GPU, i18n::format(i18n::Message::GPU_NO_WINDOW, {"Metal"}), 0);
        _layer = surface;
        _format = MTLPixelFormatBGRA8Unorm;
        _layer.device = _device;
        _layer.pixelFormat = _format;
        _layer.framebufferOnly = NO;
        _layer.drawableSize = CGSizeMake(_options.width, _options.height);
        _layer.displaySyncEnabled = _options.vsync;
        CGColorSpaceRef space = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
        _layer.colorspace = space;
        CGColorSpaceRelease(space);
    }

    // Пул на кадр: drawable возвращается слою сразу после кадра (metal_backend_impl.hpp)
    // A pool per frame: the drawable returns to the layer right after the frame (metal_backend_impl.hpp)
    bool MetalBackend::record(const FrameData& frame)
    {
        @autoreleasepool
        {
            id<MTLCommandBuffer> buffer = [_queue commandBuffer];
            GRAN_METAL_CHECK(buffer != nil, nil, "commandBuffer");
            buffer.label = @"gran.metal.frame";
            uploadFrame(frame);
            encodeUploads(buffer);
            drawFrame(buffer, frame);
            bool shown = present(buffer);
            submit(buffer);
            return shown;
        }
    }

    /*
    ** Показ: поверхность копируется в drawable слоя в том же командном буфере (порядок с отрисовкой задаёт очередь),
    ** presentDrawable показывает его после выполнения буфера. Размер drawable может отстать от поверхности на кадр
    ** (окно тянут мышью) — копируется общая часть. nil — слой не дал drawable за секунду (окно скрыто).
    **
    ** Showing: the surface is copied into the layer drawable in the same command buffer (the queue orders it with the
    ** drawing), presentDrawable shows it after the buffer runs. The drawable size may lag behind the surface by a frame
    ** (the window is dragged with the mouse) — the common part is copied. nil — the layer gave no drawable within a second
    ** (the window is hidden).
    */
    bool MetalBackend::present(id<MTLCommandBuffer> buffer)
    {
        if (_layer == nil)
            return true;
        id<CAMetalDrawable> drawable = [_layer nextDrawable];
        if (drawable == nil)
            return false;
        id<MTLTexture> target = drawable.texture;
        NSUInteger width = std::min<NSUInteger>(_surface.width, target.width);
        NSUInteger height = std::min<NSUInteger>(_surface.height, target.height);
        id<MTLBlitCommandEncoder> blit = [buffer blitCommandEncoder];
        GRAN_METAL_CHECK(blit != nil, nil, "blitCommandEncoder");
        EncoderScope scope(blit);
        blit.label = @"gran.metal.present";
        [blit copyFromTexture:_surface.resource sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0)
            sourceSize:MTLSizeMake(width, height, 1) toTexture:target destinationSlice:0 destinationLevel:0
            destinationOrigin:MTLOriginMake(0, 0, 0)];
        scope.end();
        [buffer presentDrawable:drawable];
        return true;
    }

    // Поверхность окна — BGRA8: для SnapshotSource байты R и B меняются местами; вне экрана — уже RGBA8
    // The window surface is BGRA8: for SnapshotSource the R and B bytes are swapped; offscreen it is RGBA8 already
    void toRgba(std::vector<std::uint8_t>& pixels, MTLPixelFormat format) noexcept
    {
        if (format != MTLPixelFormatBGRA8Unorm)
            return;
        for (std::size_t offset = 0; offset + 3 < pixels.size(); offset += 4)
            std::swap(pixels[offset], pixels[offset + 2]);
    }
}
