#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"
#include <cstring>

namespace gran::rhi::vk
{
    // Содержимое определяется очисткой нулями в первом кадре (clearTexture), а не копией на CPU.
    // The contents are defined by clearing with zeros in the first frame (clearTexture), not by a CPU copy.
    unsigned VulkanBackend::createTexture(const TextureDesc& desc)
    {
        unsigned slot = 0;
        if (desc.width == 0 || desc.height == 0 || desc.width > 16384 || desc.height > 16384
            || desc.width > _properties.limits.maxImageDimension2D || desc.height > _properties.limits.maxImageDimension2D)
            invalid("dimensions");
        while (slot < _textures.size() && _textures[slot].handle.get() != VK_NULL_HANDLE) ++slot;
        if (slot >= MAX_TEXTURES) invalid("texture slots");
        Image image = makeImage(desc, false);
        if (slot == _textures.size()) _textures.emplace_back();
        _textures[slot] = std::move(image);
        return slot;
    }

    /*
    ** Вычитание при проверке границ исключает переполнение x+width и y+height. Байты области
    ** копируются плотно в очередь загрузок (смещение кратно 16 — с запасом для bufferOffset)
    ** и уходят на GPU в ближайшем кадре; сама текстура на CPU не хранится.
    **
    ** Subtraction in bounds checks avoids overflowing x+width and y+height. The region bytes
    ** are copied tightly into the upload queue (the offset is a multiple of 16 — with a margin for bufferOffset)
    ** and reach the GPU in the nearest frame; the texture itself is not kept on the CPU.
    */
    void VulkanBackend::updateTexture(unsigned slot, const TextureRegion& region, const std::uint8_t* pixels, unsigned pitch)
    {
        if (slot >= _textures.size() || _textures[slot].handle.get() == VK_NULL_HANDLE) invalid("slot");
        const Image& image = _textures[slot];
        if (region.x > image.width || region.y > image.height || region.width > image.width - region.x
            || region.height > image.height - region.y || pixels == nullptr || pitch < region.width * image.bytes)
            invalid("region");
        if (region.width == 0 || region.height == 0) return;
        std::size_t row = static_cast<std::size_t>(region.width) * image.bytes;
        PendingUpload upload{slot, region, (_uploadBytes.size() + 15) / 16 * 16};
        _uploadBytes.resize(upload.offset + row * region.height);
        for (unsigned line = 0; line < region.height; ++line)
            std::memcpy(_uploadBytes.data() + upload.offset + line * row, pixels + static_cast<std::size_t>(line) * pitch, row);
        _uploads.push_back(upload);
    }

    // Последний отправленный кадр удерживает ресурс до fence; ждущие загрузки слота отменяются.
    // The last submitted frame retains the resource until its fence; the pending slot uploads are cancelled.
    void VulkanBackend::releaseTexture(unsigned slot)
    {
        if (slot >= _textures.size() || _textures[slot].handle.get() == VK_NULL_HANDLE) invalid("slot");
        unsigned last = (_current + FRAME_COUNT - 1) % FRAME_COUNT;
        _frames[last].retired.push_back(std::move(_textures[slot]));
        _textures[slot] = Image{};
        std::size_t kept = 0;
        for (const PendingUpload& upload : _uploads)
            if (upload.slot != slot) _uploads[kept++] = upload;
        _uploads.resize(kept);
    }
}
