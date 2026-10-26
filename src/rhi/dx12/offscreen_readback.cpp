#include "gran/rhi/dx12/internal/offscreen_surface.hpp"

#include <cstring>
#include <memory>

namespace gran::rhi::dx12
{
    // Цель отдыхает в COPY_SOURCE: копия в буфер чтения не требует барьера / The target rests in COPY_SOURCE: the readback copy needs no barrier
    D3D12_RESOURCE_STATES OffscreenSurface::restState() const
    {
        return D3D12_RESOURCE_STATE_COPY_SOURCE;
    }

    // Копия цели в буфер чтения с выровненным шагом / A copy of the target into the readback buffer with the aligned pitch
    void OffscreenSurface::afterDraw(ID3D12GraphicsCommandList* list)
    {
        D3D12_TEXTURE_COPY_LOCATION source{};
        D3D12_TEXTURE_COPY_LOCATION destination{};

        source.pResource = _target.get();
        source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        destination.pResource = _readback.get();
        destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        destination.PlacedFootprint.Footprint.Format = TARGET_FORMAT;
        destination.PlacedFootprint.Footprint.Width = _width;
        destination.PlacedFootprint.Footprint.Height = _height;
        destination.PlacedFootprint.Footprint.Depth = 1;
        destination.PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(_pitch);
        list->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
    }

    // Показывать нечего / There is nothing to present
    HRESULT OffscreenSurface::present(bool vsync)
    {
        (void)vsync;
        return S_OK;
    }

    // Строки без выравнивания; запись в буфер не производилась — пустой диапазон записи
    // Rows without the alignment; nothing was written into the buffer — an empty written range
    void OffscreenSurface::read(std::vector<std::uint8_t>& rgba, std::uint32_t& width, std::uint32_t& height)
    {
        D3D12_RANGE readRange{0, static_cast<SIZE_T>(_pitch * _height)};
        D3D12_RANGE noWrite{0, 0};
        std::uint8_t* mapped = nullptr;

        width = _width;
        height = _height;
        rgba.resize(static_cast<std::size_t>(_width) * _height * 4);
        GRAN_CHECK_HR(_readback->Map(0, &readRange, reinterpret_cast<void**>(&mapped)));
        for (std::uint32_t row = 0; row < _height; ++row)
            std::memcpy(rgba.data() + static_cast<std::size_t>(row) * _width * 4, mapped + row * _pitch,
                static_cast<std::size_t>(_width) * 4);
        _readback->Unmap(0, &noWrite);
    }

    // Фабрика поверхности вне экрана / The offscreen surface factory
    std::unique_ptr<Surface> createOffscreenSurface(ID3D12Device* device, std::uint32_t width, std::uint32_t height)
    {
        std::unique_ptr<OffscreenSurface> surface = std::make_unique<OffscreenSurface>();

        surface->resize(device, width, height);
        return surface;
    }
}
