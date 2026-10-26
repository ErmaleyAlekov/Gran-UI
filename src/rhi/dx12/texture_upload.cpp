#include "gran/rhi/dx12/internal/dx12_internal.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <cstring>
#include <string>

namespace gran::rhi::dx12
{
    namespace
    {
        // Байт на тексель формата / Bytes per texel of the format
        std::uint32_t texelBytes(TextureFormat format) noexcept
        {
            if (format == TextureFormat::R8)
                return 1;
            return 4;
        }

        // Шаг строки при копировании выравнивается на 256 байт (требование D3D12)
        // The row pitch for copying is aligned to 256 bytes (a D3D12 requirement)
        std::uint64_t alignedPitch(std::uint32_t width, TextureFormat format) noexcept
        {
            std::uint64_t pitch = static_cast<std::uint64_t>(width) * texelBytes(format);
            std::uint64_t alignment = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
            return (pitch + alignment - 1) & ~(alignment - 1);
        }
    }

    // С запасом на выравнивание размещения каждой загрузки / With a margin for the placement alignment of each upload
    std::uint64_t TextureTable::pendingBytes() const
    {
        std::uint64_t total = 0;

        for (const PendingUpload& upload : _pending)
            total += alignedPitch(upload.region.width, _entries[upload.slot].desc.format) * upload.region.height
                + D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
        return total;
    }

    /*
    ** Одна загрузка: строки копируются в буфер загрузки с выровненным шагом, затем
    ** CopyTextureRegion переносит их в текстуру (она уже в COPY_DEST).
    **
    ** One upload: the rows are copied into the upload buffer with the aligned pitch, then
    ** CopyTextureRegion moves them into the texture (it is already in COPY_DEST).
    */
    void TextureTable::recordOne(ID3D12GraphicsCommandList* list, UploadRing& ring, const PendingUpload& upload)
    {
        TextureEntry& entry = _entries[upload.slot];
        std::uint32_t rowBytes = upload.region.width * texelBytes(entry.desc.format);
        std::uint64_t pitch = alignedPitch(upload.region.width, entry.desc.format);
        UploadAllocation allocation;
        D3D12_TEXTURE_COPY_LOCATION destination{};
        D3D12_TEXTURE_COPY_LOCATION source{};

        if (!ring.allocate(pitch * upload.region.height, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT, allocation))
            throw GpuException(ErrorCode::GPU, i18n::format(i18n::Message::GPU_BUFFER_OVERFLOW,
                {"upload", std::to_string(pitch * upload.region.height), "?"}), 0);
        for (std::uint32_t row = 0; row < upload.region.height; ++row)
            std::memcpy(allocation.cpu + row * pitch, upload.pixels.data() + static_cast<std::size_t>(row) * rowBytes, rowBytes);
        destination.pResource = entry.resource.get();
        destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        source.pResource = allocation.resource;
        source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        source.PlacedFootprint.Offset = allocation.offset;
        source.PlacedFootprint.Footprint.Format = entry.resource->GetDesc().Format;
        source.PlacedFootprint.Footprint.Width = upload.region.width;
        source.PlacedFootprint.Footprint.Height = upload.region.height;
        source.PlacedFootprint.Footprint.Depth = 1;
        source.PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(pitch);
        list->CopyTextureRegion(&destination, upload.region.x, upload.region.y, 0, &source, nullptr);
    }

    // Все текстуры с загрузками — в COPY_DEST, копии, затем все — в чтение шейдером
    // All textures with uploads go to COPY_DEST, the copies, then all go to shader reading
    void TextureTable::recordUploads(ID3D12GraphicsCommandList* list, UploadRing& ring)
    {
        const D3D12_RESOURCE_STATES read = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        for (const PendingUpload& upload : _pending)
        {
            TextureEntry& entry = _entries[upload.slot];
            transition(list, entry.resource.get(), entry.state, D3D12_RESOURCE_STATE_COPY_DEST);
            entry.state = D3D12_RESOURCE_STATE_COPY_DEST;
        }
        for (const PendingUpload& upload : _pending)
            recordOne(list, ring, upload);
        _pending.clear();
        for (TextureEntry& entry : _entries)
        {
            if (entry.resource && entry.state != read)
            {
                transition(list, entry.resource.get(), entry.state, read);
                entry.state = read;
            }
        }
    }
}
