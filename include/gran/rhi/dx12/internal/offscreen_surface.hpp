#ifndef GRAN_RHI_DX12_INTERNAL_OFFSCREEN_SURFACE_HPP
#define GRAN_RHI_DX12_INTERNAL_OFFSCREEN_SURFACE_HPP

#include "gran/rhi/dx12/internal/dx12_internal.hpp"

namespace gran::rhi::dx12
{
    /*
    ** Цель вне экрана и буфер чтения (READBACK-куча). После каждого кадра цель копируется
    ** в буфер в том же списке команд; readPixels только отображает буфер после ожидания GPU.
    **
    ** An offscreen target and a readback buffer (the READBACK heap). After each frame the target is copied
    ** into the buffer in the same command list; readPixels only maps the buffer after waiting for the GPU.
    */
    class OffscreenSurface : public Surface
    {
    public:
        // ---- offscreen_surface.cpp ----
        void resize(ID3D12Device* device, std::uint32_t width, std::uint32_t height) override;
        ID3D12Resource* target() override;
        D3D12_CPU_DESCRIPTOR_HANDLE view() override;

        // ---- offscreen_readback.cpp ----
        D3D12_RESOURCE_STATES restState() const override;
        void afterDraw(ID3D12GraphicsCommandList* list) override;
        HRESULT present(bool vsync) override;

        // Чтение пикселей (для readOffscreenPixels) / Reading the pixels (for readOffscreenPixels)
        void read(std::vector<std::uint8_t>& rgba, std::uint32_t& width, std::uint32_t& height);

    private:
        ComRef<ID3D12DescriptorHeap> _rtvHeap;
        ComRef<ID3D12Resource> _target;
        ComRef<ID3D12Resource> _readback;
        std::uint32_t _width = 0;
        std::uint32_t _height = 0;
        std::uint64_t _pitch = 0;
    };
}

#endif
