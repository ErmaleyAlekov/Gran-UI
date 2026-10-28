#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"
#include "gran/rhi/vulkan/internal/pipeline_state.hpp"
#include "gran/rhi/shader_blobs.hpp"
#include <cstring>

namespace gran::rhi::vk
{
    // vector<uint32_t> гарантирует выравнивание SPIR-V, в отличие от встроенного массива unsigned char.
    // vector<uint32_t> guarantees SPIR-V alignment, unlike the embedded unsigned char array.
    Owned<VkShaderModule> VulkanBackend::shader(const unsigned char* bytes, std::size_t size)
    {
        if (size == 0 || size % 4 != 0) invalid("SPIR-V");
        std::vector<std::uint32_t> words(size / 4);
        std::memcpy(words.data(), bytes, size);
        auto info = structure<VkShaderModuleCreateInfo>(VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
        info.codeSize = size; info.pCode = words.data();
        Owned<VkShaderModule> module;
        GRAN_CHECK_VK(vkCreateShaderModule(_device.get(), &info, nullptr,
            module.put(std::bind(vkDestroyShaderModule, _device.get(), std::placeholders::_1, nullptr))));
        nameObject(VK_OBJECT_TYPE_SHADER_MODULE, handleValue(module.get()), "gran.shader");
        return module;
    }

    // Компиляция только при разработке; в runtime используются встроенные байт-коды.
    // Compilation happens only during development; runtime uses embedded bytecode.
    void VulkanBackend::createPipelines()
    {
        using namespace generated;
        auto vertex = shader(UI_VS_SPV, UI_VS_SPV_SIZE);
        auto pixel = shader(UI_PS_SPV, UI_PS_SPV_SIZE);
        makePipeline(0, vertex.get(), pixel.get());
        auto fullscreen = shader(FULLSCREEN_VS_SPV, FULLSCREEN_VS_SPV_SIZE);
        auto down = shader(BLUR_DOWN_PS_SPV, BLUR_DOWN_PS_SPV_SIZE);
        auto up = shader(BLUR_UP_PS_SPV, BLUR_UP_PS_SPV_SIZE);
        makePipeline(1, fullscreen.get(), down.get());
        makePipeline(2, fullscreen.get(), up.get());
    }

    // Одинаковые layout и render pass дают совместимые наборы между UI и размытием.
    // Identical layout and render pass make descriptor sets compatible between UI and blur.
    void VulkanBackend::makePipeline(unsigned index, VkShaderModule vertex, VkShaderModule pixel)
    {
        PipelineState state(index != 0);
        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT; stages[0].module = vertex; stages[0].pName = "vsMain";
        if (index != 0) stages[0].pName = "vsFullscreen";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT; stages[1].module = pixel; stages[1].pName = "psMain";
        if (index == 1) stages[1].pName = "psBlurDown";
        if (index == 2) stages[1].pName = "psBlurUp";
        state.info.stageCount = 2; state.info.pStages = stages;
        state.info.layout = _layout.get(); state.info.renderPass = _pass.get();
        GRAN_CHECK_VK(vkCreateGraphicsPipelines(_device.get(), VK_NULL_HANDLE, 1, &state.info, nullptr,
            _pipelines[index].put(std::bind(vkDestroyPipeline, _device.get(), std::placeholders::_1, nullptr))));
        nameObject(VK_OBJECT_TYPE_PIPELINE, handleValue(_pipelines[index].get()), "gran.pipeline");
    }
}
