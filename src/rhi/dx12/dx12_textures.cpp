#include "gran/rhi/dx12/internal/dx12_backend_impl.hpp"
#include "gran/rhi/dx12/internal/offscreen_surface.hpp"
#include "gran/core/gran_exception.hpp"

#include <utility>

namespace gran::rhi::dx12
{
    // Новая текстура в таблице / A new texture in the table
    std::uint32_t Dx12Backend::createTexture(const TextureDesc& desc)
    {
        return _textures.create(_context.device.get(), desc);
    }

    // Загрузка попадёт на GPU в начале следующего кадра / The upload reaches the GPU at the start of the next frame
    void Dx12Backend::updateTexture(std::uint32_t slot, const TextureRegion& region, const std::uint8_t* pixels,
        std::uint32_t rowPitch)
    {
        _textures.update(slot, region, pixels, rowPitch);
    }

    // Последний отправленный кадр мог читать текстуру: она живёт до его забора
    // The last submitted frame could read the texture: it lives until that frame's fence
    void Dx12Backend::releaseTexture(std::uint32_t slot)
    {
        _textures.release(slot, _releases, _lastSignaled);
    }

    // Только для бэкенда без окна; ждёт GPU, чтобы буфер чтения был заполнен
    // Only for a backend without a window; waits for the GPU so that the readback buffer is filled
    void Dx12Backend::readPixels(std::vector<std::uint8_t>& rgba, std::uint32_t& width, std::uint32_t& height)
    {
        if (!_offscreen)
            throwBadTexture(0, "readPixels without an offscreen surface");
        waitIdle();
        static_cast<OffscreenSurface&>(*_surface).read(rgba, width, height);
    }

    // Сообщения отладочного слоя, накопленные с прошлого вызова / The debug layer messages accumulated since the last call
    std::vector<std::string> Dx12Backend::takeDiagnostics()
    {
        // GPU validation может сообщить ошибку после submit, когда readPixels уже дождался GPU.
        // GPU validation may report after submit, once readPixels has waited for the GPU.
        if (_context.debug)
        {
            for (std::string& message : drainDebugMessages(_context.device.get()))
                _diagnostics.push_back(std::move(message));
        }
        std::vector<std::string> taken = std::move(_diagnostics);

        _diagnostics.clear();
        return taken;
    }
}
