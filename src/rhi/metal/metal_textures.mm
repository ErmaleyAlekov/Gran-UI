#include "gran/rhi/metal/internal/metal_backend_impl.hpp"

#include <cstring>

namespace gran::rhi::metal
{
    // Первый свободный логический слот; слот TEXTURE_BACKDROP зарезервирован, как в D3D11 и Vulkan
    // The first free logical slot; the TEXTURE_BACKDROP slot is reserved, as in D3D11 and Vulkan
    std::uint32_t MetalBackend::createTexture(const TextureDesc& desc)
    {
        std::uint32_t slot = 0;
        Texture texture;

        while (slot == render::TEXTURE_BACKDROP || (slot < _textures.size() && _textures[slot].resource != nil))
            ++slot;
        if (slot >= MAX_TEXTURES)
            throw GpuException(ErrorCode::LIMIT_EXCEEDED,
                i18n::format(i18n::Message::GPU_DESCRIPTOR_EXHAUSTED, {std::to_string(MAX_TEXTURES)}), 0);
        makeTexture(_device, texture, desc, false);
        // Атлас заполняется кусками, а линейная выборка задевает соседние пиксели: вся текстура сначала обнуляется
        // The atlas is filled in pieces, and linear sampling touches the neighbor pixels: the whole texture is zeroed first
        queueUpload(texture.resource, TextureRegion{0, 0, texture.width, texture.height}, nullptr, 0, texture.bytes);
        if (slot >= _textures.size())
            _textures.resize(slot + 1);
        _textures[slot] = texture;
        return slot;
    }

    // Область проверяется без переполнения (width > размер − x) до любой записи
    // The region is checked without overflow (width > size − x) before any write
    void MetalBackend::updateTexture(std::uint32_t slot, const TextureRegion& region,
        const std::uint8_t* pixels, std::uint32_t rowPitch)
    {
        if (slot >= _textures.size() || _textures[slot].resource == nil)
            invalid("slot");
        const Texture& texture = _textures[slot];
        if (region.x > texture.width || region.width > texture.width - region.x
            || region.y > texture.height || region.height > texture.height - region.y
            || rowPitch < static_cast<std::uint64_t>(region.width) * texture.bytes || pixels == nullptr)
            invalid("region");
        if (region.width == 0 || region.height == 0)
            return;
        queueUpload(texture.resource, region, pixels, rowPitch, texture.bytes);
    }

    // Освобождение слота: ожидающие загрузки и кадры в полёте держат свою ссылку на объект, слот сразу свободен
    // Releasing a slot: the pending uploads and the frames in flight hold their own reference, the slot is free at once
    void MetalBackend::releaseTexture(std::uint32_t slot)
    {
        if (slot >= _textures.size() || _textures[slot].resource == nil)
            invalid("slot");
        _textures[slot] = Texture{};
    }

    // Строки упаковываются в промежуточный буфер с шагом, кратным 4 байтам (требование копии буфер → текстура
    // на macOS); пиксели вызывающего кода после возврата больше не нужны
    // The rows are packed into a staging buffer with a step that is a multiple of 4 bytes (a macOS requirement of
    // the buffer → texture copy); the caller's pixels are no longer needed after the return
    void MetalBackend::queueUpload(id<MTLTexture> texture, const TextureRegion& region, const std::uint8_t* pixels,
        std::uint32_t rowPitch, std::uint32_t bytes)
    {
        PendingUpload upload;
        std::size_t row = static_cast<std::size_t>(region.width) * bytes;

        upload.pitch = static_cast<std::uint32_t>((row + 3) / 4 * 4);
        upload.staging = [_device newBufferWithLength:static_cast<std::size_t>(upload.pitch) * region.height
            options:MTLResourceStorageModeShared];
        GRAN_METAL_CHECK(upload.staging != nil, nil, "newBufferWithLength");
        upload.staging.label = @"gran.metal.upload";
        auto* target = static_cast<std::uint8_t*>(upload.staging.contents);
        // pixels == nullptr — обнулить область (новая текстура в частной памяти не определена)
        // pixels == nullptr — zero the region (a new texture in private memory is undefined)
        if (pixels == nullptr)
            std::memset(target, 0, static_cast<std::size_t>(upload.pitch) * region.height);
        for (std::uint32_t line = 0; pixels != nullptr && line < region.height; ++line)
            std::memcpy(target + static_cast<std::size_t>(line) * upload.pitch,
                pixels + static_cast<std::size_t>(line) * rowPitch, row);
        upload.texture = texture;
        upload.region = region;
        _uploads.push_back(upload);
    }

    // Все ожидающие загрузки — одним проходом копирования в начале командного буфера кадра
    // All the pending uploads — in one copy pass at the start of the frame command buffer
    void MetalBackend::encodeUploads(id<MTLCommandBuffer> buffer)
    {
        if (_uploads.empty())
            return;
        id<MTLBlitCommandEncoder> blit = [buffer blitCommandEncoder];
        GRAN_METAL_CHECK(blit != nil, nil, "blitCommandEncoder");
        EncoderScope scope(blit);
        blit.label = @"gran.metal.uploads";
        for (const PendingUpload& upload : _uploads)
            [blit copyFromBuffer:upload.staging sourceOffset:0 sourceBytesPerRow:upload.pitch
                sourceBytesPerImage:static_cast<NSUInteger>(upload.pitch) * upload.region.height
                sourceSize:MTLSizeMake(upload.region.width, upload.region.height, 1)
                toTexture:upload.texture destinationSlice:0 destinationLevel:0
                destinationOrigin:MTLOriginMake(upload.region.x, upload.region.y, 0)];
        scope.end();
        _uploads.clear();
    }
}
