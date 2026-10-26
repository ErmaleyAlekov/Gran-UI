#ifndef GRAN_RHI_SHADER_BLOBS_HPP
#define GRAN_RHI_SHADER_BLOBS_HPP

#include <cstddef>

namespace gran::generated
{
    /*
    ** Байт-код шейдеров из shaders/hlsl/ui.hlsl (make shaders → src/rhi/shaders_gen/<имя>.cpp).
    ** DXIL — для D3D12; SPIR-V появится при сборке с Vulkan SDK.
    **
    ** The shader byte code from shaders/hlsl/ui.hlsl (make shaders → src/rhi/shaders_gen/<name>.cpp).
    ** DXIL is for D3D12; SPIR-V appears when building with the Vulkan SDK.
    */
    extern const unsigned char UI_VS_DXIL[];
    extern const std::size_t UI_VS_DXIL_SIZE;
    extern const unsigned char UI_PS_DXIL[];
    extern const std::size_t UI_PS_DXIL_SIZE;
    extern const unsigned char FULLSCREEN_VS_DXIL[];
    extern const std::size_t FULLSCREEN_VS_DXIL_SIZE;
    extern const unsigned char BLUR_DOWN_PS_DXIL[];
    extern const std::size_t BLUR_DOWN_PS_DXIL_SIZE;
    extern const unsigned char BLUR_UP_PS_DXIL[];
    extern const std::size_t BLUR_UP_PS_DXIL_SIZE;
}

#endif
