#include "gran/rhi/dx11/internal/dx11_backend_impl.hpp"

#include <algorithm>
#include <cstring>

namespace gran::rhi::dx11
{
    // Границы проверяются до преобразования в UINT; ёмкость растёт с запасом.
    // Bounds are checked before converting to UINT; capacity grows with spare space.
    void Dx11Backend::upload(StructuredBuffer& buffer, const void* data, UINT count, UINT stride)
    {
        D3D11_BUFFER_DESC desc{};

        if (count > (128u * 1024u * 1024u) / stride)
            invalid("buffer");
        UINT bytes = std::max(1u, count) * stride;
        if (bytes > buffer.capacity)
        {
            desc.ByteWidth = std::max(bytes, 4096u);
            desc.StructureByteStride = stride;
            desc.ByteWidth = ((desc.ByteWidth + stride - 1) / stride) * stride;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            GRAN_DX11_CHECK(_device->CreateBuffer(&desc, nullptr, buffer.resource.put()));
            GRAN_DX11_CHECK(_device->CreateShaderResourceView(buffer.resource.get(), nullptr, buffer.view.put()));
            buffer.capacity = desc.ByteWidth;
            nameObject(buffer.resource.get(), "gran.dx11.structured");
            nameObject(buffer.view.get(), "gran.dx11.structured.view");
        }
        if (count == 0)
            return;
        D3D11_BOX box{0, 0, 0, count * stride, 1, 1};
        _context->UpdateSubresource(buffer.resource.get(), 0, &box, data, 0, 0);
    }

    // DISCARD даёт новую версию констант без записи поверх данных предыдущего draw.
    // DISCARD provides a new constant version without overwriting the preceding draw's data.
    void Dx11Backend::constants(const render::FrameConstants& data)
    {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        ID3D11Buffer* buffer = _constants.get();

        GRAN_DX11_CHECK(_context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
        std::memcpy(mapped.pData, &data, sizeof(data));
        _context->Unmap(buffer, 0);
        _context->VSSetConstantBuffers(0, 1, &buffer);
        _context->PSSetConstantBuffers(0, 1, &buffer);
    }
}
