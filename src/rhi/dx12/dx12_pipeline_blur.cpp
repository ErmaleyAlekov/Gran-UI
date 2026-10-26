#include "gran/rhi/dx12/internal/dx12_internal.hpp"
#include "gran/rhi/shader_blobs.hpp"

#include <climits>

namespace gran::rhi::dx12
{
    namespace
    {
        // Конвейер полноэкранного прохода с заданным пиксельным шейдером / A full screen pass pipeline with the given pixel shader
        void createPass(ID3D12Device* device, ID3D12RootSignature* root, D3D12_SHADER_BYTECODE pixel,
            ComRef<ID3D12PipelineState>& pipeline, const char* name)
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};

            desc.pRootSignature = root;
            desc.VS = D3D12_SHADER_BYTECODE{generated::FULLSCREEN_VS_DXIL, generated::FULLSCREEN_VS_DXIL_SIZE};
            desc.PS = pixel;
            desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            desc.SampleMask = UINT_MAX;
            desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
            desc.RasterizerState.DepthClipEnable = TRUE;
            desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            desc.NumRenderTargets = 1;
            desc.RTVFormats[0] = TARGET_FORMAT;
            desc.SampleDesc.Count = 1;
            GRAN_CHECK_HR(device->CreateGraphicsPipelineState(&desc, __uuidof(ID3D12PipelineState),
                reinterpret_cast<void**>(pipeline.put())));
            setDebugName(pipeline.get(), name);
        }
    }

    // Уменьшение и увеличение используют ту же корневую сигнатуру, что и интерфейс
    // The downscale and the upscale use the same root signature as the interface
    void createBlurPipelines(ID3D12Device* device, PipelineSet& pipelines)
    {
        createPass(device, pipelines.root.get(), D3D12_SHADER_BYTECODE{generated::BLUR_DOWN_PS_DXIL,
            generated::BLUR_DOWN_PS_DXIL_SIZE}, pipelines.blurDown, "gran.blurDown");
        createPass(device, pipelines.root.get(), D3D12_SHADER_BYTECODE{generated::BLUR_UP_PS_DXIL,
            generated::BLUR_UP_PS_DXIL_SIZE}, pipelines.blurUp, "gran.blurUp");
    }
}
