#include "gran/render/renderer.hpp"

namespace gran::render
{
    namespace
    {
        // Пересоздание зеркала: старый слот отдаётся бэкенду, новый заполняется целиком
        // Recreating a mirror: the old slot goes back to the backend, the new one is filled entirely
        void recreate(rhi::RenderBackend& backend, MirroredTexture& mirror, const rhi::TextureDesc& desc,
            const std::uint8_t* pixels, std::uint32_t rowPitch)
        {
            if (mirror.slot != NO_TEXTURE)
                backend.releaseTexture(mirror.slot);
            mirror.slot = backend.createTexture(desc);
            backend.updateTexture(mirror.slot, rhi::TextureRegion{0, 0, desc.width, desc.height}, pixels, rowPitch);
        }
    }

    // Атлас R8: шаг строки — сторона атласа / The R8 atlas: the row pitch is the atlas side
    void Renderer::syncAtlas()
    {
        std::uint32_t size = _atlas.size();
        rhi::TextureRegion dirty;

        if (_atlasTexture.slot == NO_TEXTURE || _atlasTexture.generation != _atlas.generation())
        {
            recreate(_backend, _atlasTexture, rhi::TextureDesc{size, size, rhi::TextureFormat::R8, "gran.glyphAtlas"},
                _atlas.pixels(), size);
            _atlasTexture.generation = _atlas.generation();
            _atlas.takeDirty(dirty.x, dirty.y, dirty.width, dirty.height);
            return;
        }
        if (_atlas.takeDirty(dirty.x, dirty.y, dirty.width, dirty.height))
            _backend.updateTexture(_atlasTexture.slot, dirty, _atlas.pixels() + static_cast<std::size_t>(dirty.y) * size + dirty.x,
                size);
    }

    // Рампы RGBA8: по 256 текселей в строке / The RGBA8 ramps: 256 texels per row
    void Renderer::syncRamps()
    {
        std::uint32_t rows = _ramps.capacity();
        std::uint32_t pitch = RAMP_WIDTH * 4;
        std::uint32_t first = 0;
        std::uint32_t count = 0;

        if (_rampTexture.slot == NO_TEXTURE || _rampTexture.generation != _ramps.generation())
        {
            recreate(_backend, _rampTexture, rhi::TextureDesc{RAMP_WIDTH, rows, rhi::TextureFormat::RGBA8, "gran.gradientRamps"},
                _ramps.pixels(), pitch);
            _rampTexture.generation = _ramps.generation();
            _ramps.takeDirty(first, count);
            return;
        }
        if (_ramps.takeDirty(first, count))
            _backend.updateTexture(_rampTexture.slot, rhi::TextureRegion{0, first, RAMP_WIDTH, count},
                _ramps.pixels() + static_cast<std::size_t>(first) * pitch, pitch);
    }
}
