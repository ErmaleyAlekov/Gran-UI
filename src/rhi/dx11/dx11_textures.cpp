#include "gran/rhi/dx11/internal/dx11_backend_impl.hpp"

namespace gran::rhi::dx11
{
    // Слоты — логические идентификаторы, поэтому число текстур не ограничено регистрами SM5.
    // Slots are logical identifiers, so SM5 registers do not limit the texture count.
    std::uint32_t Dx11Backend::createTexture(const TextureDesc& desc)
    {
        UINT slot = 0;
        Texture texture;

        while (slot == render::TEXTURE_BACKDROP || (slot < _textures.size() && _textures[slot].resource))
            ++slot;
        if (slot >= MAX_TEXTURES)
            throw GpuException(ErrorCode::LIMIT_EXCEEDED,
                i18n::format(i18n::Message::GPU_DESCRIPTOR_EXHAUSTED, {std::to_string(MAX_TEXTURES)}), 0);
        makeTexture(_device.get(), texture, desc, false);
        if (slot >= _textures.size())
            _textures.resize(slot + 1);
        _textures[slot] = std::move(texture);
        return slot;
    }

    // UpdateSubresource копирует память CPU до возврата; контекст разрешает GPU-зависимости.
    // UpdateSubresource copies CPU memory before returning; the context resolves GPU dependencies.
    void Dx11Backend::updateTexture(std::uint32_t slot, const TextureRegion& region,
        const std::uint8_t* pixels, std::uint32_t rowPitch)
    {
        if (slot >= _textures.size() || !_textures[slot].resource)
            invalid("slot");
        Texture& texture = _textures[slot];
        if (region.x > texture.width || region.width > texture.width - region.x
            || region.y > texture.height || region.height > texture.height - region.y
            || rowPitch < static_cast<std::uint64_t>(region.width) * texture.bytes || pixels == nullptr)
            invalid("region");
        if (region.width == 0 || region.height == 0)
            return;
        D3D11_BOX box{region.x, region.y, 0, region.x + region.width, region.y + region.height, 1};
        _context->UpdateSubresource(texture.resource.get(), 0, &box, pixels, rowPitch, 0);
    }

    // Unbind снимает ссылки контекста; драйвер удерживает уже отправленные команды.
    // Unbind removes context references; the driver retains already submitted commands.
    void Dx11Backend::releaseTexture(std::uint32_t slot)
    {
        if (slot >= _textures.size() || !_textures[slot].resource)
            invalid("slot");
        ID3D11ShaderResourceView* empty[4] = {};
        _context->PSSetShaderResources(2, 4, empty);
        _textures[slot] = Texture{};
    }
}
