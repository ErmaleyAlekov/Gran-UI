#include "gran/rhi/dx11/internal/dx11_backend_impl.hpp"

#include <cstring>
#include <utility>

namespace gran::rhi::dx11
{
    // Map(STAGING) ждёт копию, шаг строки учитывается при копировании в плотный RGBA.
    // Map(STAGING) waits for the copy; row pitch is honored when copying into packed RGBA.
    void Dx11Backend::readPixels(std::vector<std::uint8_t>& rgba, std::uint32_t& width, std::uint32_t& height)
    {
        ComRef<ID3D11Texture2D> staging;
        D3D11_TEXTURE2D_DESC desc{};
        D3D11_MAPPED_SUBRESOURCE mapped{};

        _surface.resource->GetDesc(&desc);
        desc.BindFlags = desc.MiscFlags = 0;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        GRAN_DX11_CHECK(_device->CreateTexture2D(&desc, nullptr, staging.put()));
        nameObject(staging.get(), "gran.dx11.readback");
        width = desc.Width;
        height = desc.Height;
        rgba.resize(static_cast<std::size_t>(width) * height * 4);
        _context->CopyResource(staging.get(), _surface.resource.get());
        GRAN_DX11_CHECK(_context->Map(staging.get(), 0, D3D11_MAP_READ, 0, &mapped));
        for (UINT row = 0; row < height; ++row)
            std::memcpy(rgba.data() + static_cast<std::size_t>(row) * width * 4,
                static_cast<const std::uint8_t*>(mapped.pData) + static_cast<std::size_t>(row) * mapped.RowPitch, width * 4);
        _context->Unmap(staging.get(), 0);
    }

    // Любое сообщение слоя доступно вызывающему; тесты считают его провалом.
    // Every layer message is available to the caller; tests treat it as failure.
    std::vector<std::string> Dx11Backend::takeDiagnostics()
    {
        std::vector<std::string> messages;

        if (!_info)
            return messages;
        UINT64 count = _info->GetNumStoredMessagesAllowedByRetrievalFilter();
        for (UINT64 index = 0; index < count; ++index)
        {
            SIZE_T size = 0;
            GRAN_DX11_CHECK(_info->GetMessage(index, nullptr, &size));
            std::vector<std::uint8_t> storage(size);
            auto* message = reinterpret_cast<D3D11_MESSAGE*>(storage.data());
            GRAN_DX11_CHECK(_info->GetMessage(index, message, &size));
            messages.emplace_back(message->pDescription);
        }
        _info->ClearStoredMessages();
        return messages;
    }
}
