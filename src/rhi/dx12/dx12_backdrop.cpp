#include "gran/rhi/dx12/internal/dx12_backend_impl.hpp"

#include <algorithm>
#include <cmath>

namespace gran::rhi::dx12
{
    namespace
    {
        // Переход уровня с запоминанием состояния / A level transition remembering the state
        void moveLevel(ID3D12GraphicsCommandList* list, BackdropChain& chain, std::uint32_t level, D3D12_RESOURCE_STATES state)
        {
            transition(list, chain.levels[level].get(), chain.states[level], state);
            chain.states[level] = state;
        }

        // Возврат к рисованию интерфейса в цель кадра / Returning to drawing the interface into the frame target
        void restoreTarget(ID3D12GraphicsCommandList* list, Surface& surface, ID3D12PipelineState* pipeline)
        {
            D3D12_RESOURCE_DESC desc = surface.target()->GetDesc();
            D3D12_CPU_DESCRIPTOR_HANDLE view = surface.view();
            D3D12_VIEWPORT viewport{0.0f, 0.0f, static_cast<float>(desc.Width), static_cast<float>(desc.Height), 0.0f, 1.0f};
            D3D12_RECT scissor{0, 0, static_cast<LONG>(desc.Width), static_cast<LONG>(desc.Height)};

            list->SetPipelineState(pipeline);
            list->OMSetRenderTargets(1, &view, FALSE, nullptr);
            list->RSSetViewports(1, &viewport);
            list->RSSetScissorRects(1, &scissor);
            list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        }
    }

    /*
    ** Подложка перед сегментом: копия цели в уровень 0, уменьшения до уровня n (n растёт
    ** с радиусом), увеличения обратно до уровня 1. Затем цель снова привязывается для рисования.
    **
    ** The backdrop before a segment: a copy of the target into level 0, halvings down to level n (n grows
    ** with the radius), doublings back up to level 1. Then the target is bound for drawing again.
    */
    void Dx12Backend::recordBackdrop(float radius)
    {
        ID3D12Resource* target = _surface->target();
        std::uint32_t passes = static_cast<std::uint32_t>(std::lround(std::log2(std::max(2.0f, radius / 1.5f))));

        passes = std::min(BLUR_LEVELS - 1, std::max(1u, passes));
        ensureBackdrop();
        transition(_list.get(), target, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
        moveLevel(_list.get(), _backdrop, 0, D3D12_RESOURCE_STATE_COPY_DEST);
        _list->CopyResource(_backdrop.levels[0].get(), target);
        transition(_list.get(), target, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        moveLevel(_list.get(), _backdrop, 0, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        for (std::uint32_t level = 1; level <= passes; ++level)
            blurPass(level - 1, level, _pipelines.blurDown.get());
        for (std::uint32_t level = passes - 1; level >= 1; --level)
            blurPass(level + 1, level, _pipelines.blurUp.get());
        restoreTarget(_list.get(), *_surface, _pipelines.ui.get());
    }

    // Один проход: источник читается шейдером, приёмник — цель; треугольник на весь уровень
    // One pass: the source is read by the shader, the destination is the target; a triangle over the whole level
    void Dx12Backend::blurPass(std::uint32_t source, std::uint32_t destination, ID3D12PipelineState* pipeline)
    {
        D3D12_RESOURCE_DESC desc = _backdrop.levels[destination]->GetDesc();
        D3D12_RESOURCE_DESC from = _backdrop.levels[source]->GetDesc();
        D3D12_VIEWPORT viewport{0.0f, 0.0f, static_cast<float>(desc.Width), static_cast<float>(desc.Height), 0.0f, 1.0f};
        D3D12_RECT scissor{0, 0, static_cast<LONG>(desc.Width), static_cast<LONG>(desc.Height)};
        D3D12_CPU_DESCRIPTOR_HANDLE view = backdropView(_backdrop, destination);
        render::FrameConstants constants{};

        moveLevel(_list.get(), _backdrop, source, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        moveLevel(_list.get(), _backdrop, destination, D3D12_RESOURCE_STATE_RENDER_TARGET);
        constants.viewport[0] = viewport.Width;
        constants.viewport[1] = viewport.Height;
        constants.inverseViewport[0] = 1.0f / static_cast<float>(from.Width);
        constants.inverseViewport[1] = 1.0f / static_cast<float>(from.Height);
        constants.instanceBase = BLUR_SLOT_FIRST + source;
        _list->SetPipelineState(pipeline);
        _list->OMSetRenderTargets(1, &view, FALSE, nullptr);
        _list->RSSetViewports(1, &viewport);
        _list->RSSetScissorRects(1, &scissor);
        _list->SetGraphicsRoot32BitConstants(0, render::FRAME_CONSTANT_COUNT, &constants, 0);
        _list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        _list->DrawInstanced(3, 1, 0, 0);
        moveLevel(_list.get(), _backdrop, destination, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
}
