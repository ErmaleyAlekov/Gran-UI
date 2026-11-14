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

struct psBlurDown_out
{
    float4 out_var_SV_Target [[color(0)]];
};

struct psBlurDown_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
};

fragment psBlurDown_out psBlurDown(psBlurDown_in in [[stage_in]], constant type_ConstantBuffer_FrameConstants& frame [[buffer(0)]], texture2d<float> imageTexture [[texture(2)]], sampler linearSampler [[sampler(0)]])
{
    psBlurDown_out out = {};
    float2 _36 = frame.inverseViewport * 0.5;
    float2 _55 = float2(_36.x, -_36.y);
    out.out_var_SV_Target = (((((imageTexture.sample(linearSampler, in.in_var_TEXCOORD0, level(0.0)) * 4.0) + imageTexture.sample(linearSampler, (in.in_var_TEXCOORD0 - _36), level(0.0))) + imageTexture.sample(linearSampler, (in.in_var_TEXCOORD0 + _36), level(0.0))) + imageTexture.sample(linearSampler, (in.in_var_TEXCOORD0 + _55), level(0.0))) + imageTexture.sample(linearSampler, (in.in_var_TEXCOORD0 - _55), level(0.0))) * float4(0.125);
    return out;
}

