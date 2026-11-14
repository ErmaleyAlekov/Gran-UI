#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_ConstantBuffer_FrameConstants
{
    float2 viewport;
    float2 inverseViewport;
    uint instanceBase;
    uint clipCount;
    uint textureCount;
    uint rampRows;
    uint atlasSlot;
    uint rampSlot;
    uint2 reserved;
};

struct psBlurUp_out
{
    float4 out_var_SV_Target [[color(0)]];
};

struct psBlurUp_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
};

fragment psBlurUp_out psBlurUp(psBlurUp_in in [[stage_in]], constant type_ConstantBuffer_FrameConstants& frame [[buffer(0)]], texture2d<float> imageTexture [[texture(2)]], sampler linearSampler [[sampler(0)]])
{
    psBlurUp_out out = {};
    float2 _37 = frame.inverseViewport * 0.5;
    float _39 = _37.x;
    float _46 = -_39;
    float _47 = _37.y;
    float _75 = -_47;
    out.out_var_SV_Target = (((((((imageTexture.sample(linearSampler, (in.in_var_TEXCOORD0 + float2(_39 * (-2.0), 0.0)), level(0.0)) + (imageTexture.sample(linearSampler, (in.in_var_TEXCOORD0 + float2(_46, _47)), level(0.0)) * 2.0)) + imageTexture.sample(linearSampler, (in.in_var_TEXCOORD0 + float2(0.0, _47 * 2.0)), level(0.0))) + (imageTexture.sample(linearSampler, (in.in_var_TEXCOORD0 + _37), level(0.0)) * 2.0)) + imageTexture.sample(linearSampler, (in.in_var_TEXCOORD0 + float2(_39 * 2.0, 0.0)), level(0.0))) + (imageTexture.sample(linearSampler, (in.in_var_TEXCOORD0 + float2(_39, _75)), level(0.0)) * 2.0)) + imageTexture.sample(linearSampler, (in.in_var_TEXCOORD0 + float2(0.0, _47 * (-2.0))), level(0.0))) + (imageTexture.sample(linearSampler, (in.in_var_TEXCOORD0 + float2(_46, _75)), level(0.0)) * 2.0)) * float4(0.083333335816860198974609375);
    return out;
}

