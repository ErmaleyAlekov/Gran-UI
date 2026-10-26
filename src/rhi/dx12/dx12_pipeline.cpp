#include "gran/rhi/dx12/internal/dx12_internal.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/rhi/shader_blobs.hpp"

#include <climits>
#include <string>

namespace gran::rhi::dx12
{
    namespace
    {
        // Параметры корневой сигнатуры (совпадают с ui_common.hlsli) / The root signature parameters (match ui_common.hlsli)
        void fillRootParameters(D3D12_ROOT_PARAMETER* parameters, const D3D12_DESCRIPTOR_RANGE* textures)
        {
            parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            parameters[0].Constants.Num32BitValues = render::FRAME_CONSTANT_COUNT;
            parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
            parameters[1].Descriptor.ShaderRegister = 0;
            parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
            parameters[2].Descriptor.ShaderRegister = 1;
            parameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            parameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            parameters[3].DescriptorTable.NumDescriptorRanges = 1;
            parameters[3].DescriptorTable.pDescriptorRanges = textures;
            parameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        }

        /*
        ** b0 — 8 констант кадра; t0, t1 — корневые SRV экземпляров и отсечений (без дескрипторов,
        ** адрес меняется каждый кадр); t0…∞ в space1 — таблица текстур; s0 — линейный сэмплер.
        **
        ** b0 — 8 frame constants; t0, t1 — the root SRVs of instances and clips (no descriptors,
        ** the address changes every frame); t0…∞ in space1 — the texture table; s0 — a linear sampler.
        */
        void createRootSignature(ID3D12Device* device, ComRef<ID3D12RootSignature>& root)
        {
            D3D12_DESCRIPTOR_RANGE textures{D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, 0, 1, 0};
            D3D12_ROOT_PARAMETER parameters[4] = {};
            D3D12_STATIC_SAMPLER_DESC sampler{};
            D3D12_ROOT_SIGNATURE_DESC desc{};
            ComRef<ID3DBlob> blob;
            ComRef<ID3DBlob> error;

            fillRootParameters(parameters, &textures);
            sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            sampler.MaxLOD = D3D12_FLOAT32_MAX;
            sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            desc.NumParameters = 4;
            desc.pParameters = parameters;
            desc.NumStaticSamplers = 1;
            desc.pStaticSamplers = &sampler;
            GRAN_CHECK_HR(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, blob.put(), error.put()));
            GRAN_CHECK_HR(device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(),
                __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(root.put())));
            setDebugName(root.get(), "gran.rootSignature");
        }

        // Предумноженная альфа: цвет = источник + приёмник × (1 − альфа источника)
        // Premultiplied alpha: color = source + destination × (1 − source alpha)
        D3D12_RENDER_TARGET_BLEND_DESC premultipliedBlend() noexcept
        {
            D3D12_RENDER_TARGET_BLEND_DESC blend{};

            blend.BlendEnable = TRUE;
            blend.SrcBlend = D3D12_BLEND_ONE;
            blend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            blend.BlendOp = D3D12_BLEND_OP_ADD;
            blend.SrcBlendAlpha = D3D12_BLEND_ONE;
            blend.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
            blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
            blend.LogicOp = D3D12_LOGIC_OP_NOOP;
            blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            return blend;
        }
    }

    // Состояние основного конвейера интерфейса: полоса из 4 вершин без буфера вершин
    // The main interface pipeline state: a strip of 4 vertices without a vertex buffer
    void createPipelines(ID3D12Device* device, PipelineSet& pipelines)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};

        createRootSignature(device, pipelines.root);
        desc.pRootSignature = pipelines.root.get();
        desc.VS = D3D12_SHADER_BYTECODE{generated::UI_VS_DXIL, generated::UI_VS_DXIL_SIZE};
        desc.PS = D3D12_SHADER_BYTECODE{generated::UI_PS_DXIL, generated::UI_PS_DXIL_SIZE};
        desc.BlendState.RenderTarget[0] = premultipliedBlend();
        desc.SampleMask = UINT_MAX;
        desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        desc.RasterizerState.DepthClipEnable = TRUE;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = TARGET_FORMAT;
        desc.SampleDesc.Count = 1;
        GRAN_CHECK_HR(device->CreateGraphicsPipelineState(&desc, __uuidof(ID3D12PipelineState),
            reinterpret_cast<void**>(pipelines.ui.put())));
        setDebugName(pipelines.ui.get(), "gran.uiPipeline");
    }
}
