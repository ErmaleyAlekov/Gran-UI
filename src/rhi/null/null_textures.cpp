#include "gran/rhi/null/null_backend.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <string>

namespace gran::rhi
{
    namespace
    {
        // Ошибка с текстурой — как у настоящих бэкендов (dx12_check.cpp) / A texture error — as the real backends do (dx12_check.cpp)
        [[noreturn]] void throwBadTexture(std::uint32_t slot, const char* reason)
        {
            throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::GPU_BAD_TEXTURE,
                {std::to_string(slot), reason}));
        }

        // Живая ли текстура в слоте: освобождённая хранится с нулевой шириной / Whether the texture in the slot is alive: a released one is kept with zero width
        bool alive(const std::vector<TextureDesc>& textures, std::uint32_t slot) noexcept
        {
            return slot < textures.size() && textures[slot].width != 0;
        }
    }

    // Первый свободный слот или новый; пустой размер — ошибка, как у GPU / The first free slot or a new one; an empty size is an error, as with a GPU
    std::uint32_t NullBackend::createTexture(const TextureDesc& desc)
    {
        std::uint32_t slot = 0;

        if (desc.width == 0 || desc.height == 0)
            throwBadTexture(0, "empty texture size");
        while (slot < _textures.size() && _textures[slot].width != 0)
            ++slot;
        if (slot == _textures.size())
            _textures.push_back(desc);
        else
            _textures[slot] = desc;
        ++_stats.liveTextures;
        return slot;
    }

    /*
    ** Пикселей не копируем, но проверяем то же, что проверил бы GPU-бэкенд до записи: слот жив, участок
    ** внутри текстуры (без переполнения: сравнение с остатком), шаг строки не меньше ширины участка.
    **
    ** No pixels are copied, but the same is checked as a GPU backend would check before writing: the slot is alive, the region
    ** is inside the texture (without overflow: a comparison with the remainder), the row pitch is not below the region width.
    */
    void NullBackend::updateTexture(std::uint32_t slot, const TextureRegion& region, const std::uint8_t* pixels,
        std::uint32_t rowPitch)
    {
        std::uint32_t bytes = 4;

        if (!alive(_textures, slot))
            throwBadTexture(slot, "update of a missing texture");
        const TextureDesc& desc = _textures[slot];
        if (desc.format == TextureFormat::R8)
            bytes = 1;
        if (region.x > desc.width || region.width > desc.width - region.x
            || region.y > desc.height || region.height > desc.height - region.y)
            throwBadTexture(slot, "update region outside the texture");
        if (pixels == nullptr || rowPitch / bytes < region.width)
            throwBadTexture(slot, "update without pixels or with a short row pitch");
    }

    void NullBackend::releaseTexture(std::uint32_t slot)
    {
        if (!alive(_textures, slot))
            throwBadTexture(slot, "release of a missing texture");
        _textures[slot] = TextureDesc{0, 0, TextureFormat::RGBA8, "released"};
        --_stats.liveTextures;
    }
}
