#include "gran/rhi/metal/internal/metal_backend_impl.hpp"

#include <cstring>

namespace gran::rhi::metal
{
    // Чтение кадра: поверхность (частная память) копируется в общий буфер со строкой, кратной 256 байтам,
    // затем строки переписываются без выравнивания, сверху вниз — формат SnapshotSource
    // Reading the frame: the surface (private memory) is copied into a shared buffer with a row that is a multiple
    // of 256 bytes, then the rows are rewritten unaligned, top to bottom — the SnapshotSource format
    void MetalBackend::readPixels(std::vector<std::uint8_t>& rgba, std::uint32_t& width, std::uint32_t& height)
    {
        width = _surface.width;
        height = _surface.height;
        std::size_t row = static_cast<std::size_t>(width) * 4;
        std::size_t pitch = (row + 255) / 256 * 256;
        id<MTLBuffer> staging = [_device newBufferWithLength:pitch * height options:MTLResourceStorageModeShared];
        GRAN_METAL_CHECK(staging != nil, nil, "newBufferWithLength");
        staging.label = @"gran.metal.readback";
        id<MTLCommandBuffer> buffer = [_queue commandBuffer];
        GRAN_METAL_CHECK(buffer != nil, nil, "commandBuffer");
        buffer.label = @"gran.metal.readback";
        id<MTLBlitCommandEncoder> blit = [buffer blitCommandEncoder];
        GRAN_METAL_CHECK(blit != nil, nil, "blitCommandEncoder");
        EncoderScope scope(blit);
        [blit copyFromTexture:_surface.resource sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0)
            sourceSize:MTLSizeMake(width, height, 1) toBuffer:staging destinationOffset:0
            destinationBytesPerRow:pitch destinationBytesPerImage:pitch * height];
        scope.end();
        [buffer commit];
        [buffer waitUntilCompleted];
        GRAN_METAL_CHECK(buffer.status == MTLCommandBufferStatusCompleted, buffer.error, "readback");
        rgba.resize(row * height);
        const auto* source = static_cast<const std::uint8_t*>(staging.contents);
        for (std::uint32_t line = 0; line < height; ++line)
            std::memcpy(rgba.data() + line * row, source + line * pitch, row);
        toRgba(rgba, _format);
    }

    // Последний отправленный командный буфер завершён — значит, завершены и все прежние (одна очередь)
    // The last submitted command buffer is completed — so all the earlier ones are too (one queue)
    void MetalBackend::waitIdle()
    {
        if (_last == nil)
            return;
        [_last waitUntilCompleted];
        _last = nil;
    }

    // Ошибки командных буферов, собранные обработчиками завершения (сообщения отладочного слоя Metal идут в stderr)
    // The command buffer errors collected by the completion handlers (the Metal debug layer messages go to stderr)
    std::vector<std::string> MetalBackend::takeDiagnostics()
    {
        std::lock_guard<std::mutex> lock(_shared->mutex);
        std::vector<std::string> messages;

        messages.swap(_shared->messages);
        return messages;
    }

    AdapterInfo MetalBackend::adapterInfo() const
    {
        AdapterInfo info;

        info.api = "Metal";
        info.name = utf8(_device.name);
        info.driver = utf8([NSProcessInfo processInfo].operatingSystemVersionString);
        info.software = false;
        return info;
    }

    std::string MetalBackend::lostReason() const
    {
        std::lock_guard<std::mutex> lock(_shared->mutex);
        return _shared->lostReason;
    }
}
