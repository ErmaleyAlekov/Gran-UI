#ifndef GRAN_RHI_SHADER_BLOBS_HPP
#define GRAN_RHI_SHADER_BLOBS_HPP

#include <cstddef>

namespace gran::generated
{
    /*
    ** Байт-код шейдеров из shaders/hlsl/ui.hlsl (make shaders → src/rhi/shaders_gen/<имя>.cpp).
    ** DXIL — для D3D12; SPIR-V — для Vulkan на Linux.
    **
    ** The shader byte code from shaders/hlsl/ui.hlsl (make shaders → src/rhi/shaders_gen/<name>.cpp).
    ** DXIL is for D3D12; SPIR-V is for Vulkan on Linux.
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
    // DXBC SM5 для D3D11 из того же HLSL / DXBC SM5 for D3D11 from the same HLSL
    extern const unsigned char UI_VS_DXBC[];
    extern const std::size_t UI_VS_DXBC_SIZE;
    extern const unsigned char UI_PS_DXBC[];
    extern const std::size_t UI_PS_DXBC_SIZE;
    extern const unsigned char FULLSCREEN_VS_DXBC[];
    extern const std::size_t FULLSCREEN_VS_DXBC_SIZE;
    extern const unsigned char BLUR_DOWN_PS_DXBC[];
    extern const std::size_t BLUR_DOWN_PS_DXBC_SIZE;
    extern const unsigned char BLUR_UP_PS_DXBC[];
    extern const std::size_t BLUR_UP_PS_DXBC_SIZE;
    // SPIR-V Vulkan 1.0 из того же HLSL. / Vulkan 1.0 SPIR-V from the same HLSL.
    extern const unsigned char UI_VS_SPV[];
    extern const std::size_t UI_VS_SPV_SIZE;
    extern const unsigned char UI_PS_SPV[];
    extern const std::size_t UI_PS_SPV_SIZE;
    extern const unsigned char FULLSCREEN_VS_SPV[];
    extern const std::size_t FULLSCREEN_VS_SPV_SIZE;
    extern const unsigned char BLUR_DOWN_PS_SPV[];
    extern const std::size_t BLUR_DOWN_PS_SPV_SIZE;
    extern const unsigned char BLUR_UP_PS_SPV[];
    extern const std::size_t BLUR_UP_PS_SPV_SIZE;
}

#endif
