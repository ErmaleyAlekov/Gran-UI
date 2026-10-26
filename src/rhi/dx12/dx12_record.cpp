#include "gran/rhi/dx12/internal/dx12_backend_impl.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <cstring>
#include <string>

namespace gran::rhi::dx12
{
    namespace
    {
        // Копия массива в кольцо; адрес корневого SRV выравнивается на 256 байт
        // Copying an array into the ring; the root SRV address is aligned to 256 bytes
        D3D12_GPU_VIRTUAL_ADDRESS copyToRing(UploadRing& ring, const void* data, std::uint64_t size, const char* name)
        {
            UploadAllocation allocation;

            if (!ring.allocate(size, 256, allocation))
                throw GpuException(ErrorCode::GPU, i18n::format(i18n::Message::GPU_BUFFER_OVERFLOW,
                    {name, std::to_string(size), "ring"}), 0);
            if (size > 0)
                std::memcpy(allocation.cpu, data, static_cast<std::size_t>(size));
            return allocation.gpu;
        }

        // Константы сегмента / The segment constants
        render::FrameConstants segmentConstants(const D3D12_RESOURCE_DESC& target, const FrameData& frame,
            std::uint32_t first, std::uint32_t textures)
        {
            render::FrameConstants constants{};

            constants.viewport[0] = static_cast<float>(target.Width);
            constants.viewport[1] = static_cast<float>(target.Height);
            constants.inverseViewport[0] = 1.0f / constants.viewport[0];
            constants.inverseViewport[1] = 1.0f / constants.viewport[1];
            constants.instanceBase = first;
            constants.clipCount = frame.clipCount;
            constants.textureCount = textures;
            constants.rampRows = frame.rampRows;
            constants.atlasSlot = frame.atlasSlot;
            constants.rampSlot = frame.rampSlot;
            return constants;
        }
    }

    namespace
    {
        // Корневая сигнатура, таблица текстур, данные кадра и топология / The root signature, the texture table, the frame data and the topology
        void bindResources(ID3D12GraphicsCommandList* list, const PipelineSet& pipelines, ID3D12DescriptorHeap* heap,
            const FrameAddresses& addresses)
        {
            ID3D12DescriptorHeap* heaps[] = {heap};

            list->SetGraphicsRootSignature(pipelines.root.get());
            list->SetDescriptorHeaps(1, heaps);
            list->SetGraphicsRootShaderResourceView(1, addresses.instances);
            list->SetGraphicsRootShaderResourceView(2, addresses.clips);
            list->SetGraphicsRootDescriptorTable(3, heap->GetGPUDescriptorHandleForHeapStart());
            list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        }
    }

    // Загрузки текстур, затем экземпляры и отсечения (хотя бы один экземпляр-заглушка для адреса)
    // The texture uploads, then the instances and clips (at least one placeholder instance for the address)
    FrameAddresses Dx12Backend::uploadFrame(const FrameData& frame, FrameContext& context)
    {
        FrameAddresses addresses;
        render::GpuInstance placeholder{};

        _textures.recordUploads(_list.get(), context.ring);
        if (frame.instanceCount == 0)
            addresses.instances = copyToRing(context.ring, &placeholder, sizeof(placeholder), "instances");
        else
            addresses.instances = copyToRing(context.ring, frame.instances,
                static_cast<std::uint64_t>(frame.instanceCount) * sizeof(render::GpuInstance), "instances");
        addresses.clips = copyToRing(context.ring, frame.clips,
            static_cast<std::uint64_t>(frame.clipCount) * sizeof(render::GpuClip), "clips");
        return addresses;
    }

    /*
    ** Цель: покой → RENDER_TARGET, очистка, привязки, по вызову на сегмент, обратно в покой.
    ** Экземпляры берутся из frame.instanceBase, а не из StartInstanceLocation (см. ui.hlsl).
    **
    ** The target: rest → RENDER_TARGET, the clear, the bindings, one call per segment, back to rest.
    ** The instances are taken from frame.instanceBase, not from StartInstanceLocation (see ui.hlsl).
    ** Подложка перед сегментом сама возвращает цель, конвейер и топологию (dx12_backdrop.cpp).
    ** A backdrop before a segment restores the target, the pipeline and the topology itself (dx12_backdrop.cpp).
    */
    void Dx12Backend::recordDraws(const FrameData& frame, const FrameAddresses& addresses)
    {
        ID3D12Resource* target = _surface->target();
        D3D12_RESOURCE_DESC desc = target->GetDesc();
        D3D12_CPU_DESCRIPTOR_HANDLE view = _surface->view();
        D3D12_VIEWPORT viewport{0.0f, 0.0f, static_cast<float>(desc.Width), static_cast<float>(desc.Height), 0.0f, 1.0f};
        D3D12_RECT scissor{0, 0, static_cast<LONG>(desc.Width), static_cast<LONG>(desc.Height)};

        transition(_list.get(), target, _surface->restState(), D3D12_RESOURCE_STATE_RENDER_TARGET);
        _list->OMSetRenderTargets(1, &view, FALSE, nullptr);
        _list->ClearRenderTargetView(view, frame.clearColor, 0, nullptr);
        _list->RSSetViewports(1, &viewport);
        _list->RSSetScissorRects(1, &scissor);
        bindResources(_list.get(), _pipelines, _textures.heap(), addresses);
        for (std::uint32_t index = 0; index < frame.segmentCount; ++index)
        {
            const FrameSegment& segment = frame.segments[index];
            if (segment.backdropBefore)
                recordBackdrop(segment.backdropRadius);
            render::FrameConstants constants = segmentConstants(desc, frame, segment.first, _textures.usedSlots());
            _list->SetGraphicsRoot32BitConstants(0, render::FRAME_CONSTANT_COUNT, &constants, 0);
            if (segment.count > 0)
                _list->DrawInstanced(4, segment.count, 0, 0);
        }
        transition(_list.get(), target, D3D12_RESOURCE_STATE_RENDER_TARGET, _surface->restState());
        _surface->afterDraw(_list.get());
    }
}
