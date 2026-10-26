#include "gran/rhi/dx12/internal/dx12_internal.hpp"

#include <cstring>
#include <utility>

namespace gran::rhi::dx12
{
    /*
    ** Границы проверяются до копирования, каждое сравнение — без переполнения суммы
    ** (x > ширины и width > ширины − x вместо x + width > ширины).
    **
    ** The bounds are checked before copying, each comparison without sum overflow
    ** (x > width and width > width − x instead of x + width > width).
    */
    void TextureTable::update(std::uint32_t slot, const TextureRegion& region, const std::uint8_t* pixels,
        std::uint32_t rowPitch)
    {
        if (slot >= TEXTURE_SLOTS || !_entries[slot].resource || pixels == nullptr)
            throwBadTexture(slot, "slot");
        const TextureDesc& desc = _entries[slot].desc;
        std::size_t texel = 4;
        if (desc.format == TextureFormat::R8)
            texel = 1;
        if (region.width == 0 || region.height == 0 || region.x > desc.width || region.y > desc.height
            || region.width > desc.width - region.x || region.height > desc.height - region.y
            || rowPitch < region.width * texel)
            throwBadTexture(slot, "region");
        std::size_t rowBytes = region.width * texel;
        PendingUpload upload{slot, region, std::vector<std::uint8_t>(rowBytes * region.height)};
        for (std::uint32_t row = 0; row < region.height; ++row)
            std::memcpy(upload.pixels.data() + row * rowBytes, pixels + static_cast<std::size_t>(row) * rowPitch, rowBytes);
        _pending.push_back(std::move(upload));
    }
}
