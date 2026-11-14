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

struct GpuInstance
{
    float4 rect;
    float4 radii;
    float4 border;
    float4 transform;
    float2 translate;
    float opacity;
    float param;
    uint kind;
    uint clip;
    uint fill;
    uint stroke;
    float4 extra;
    uint4 paint;
};

struct type_StructuredBuffer_GpuInstance
{
    GpuInstance _m0[1];
};

struct vsMain_out
{
    float2 out_var_LOCAL [[user(locn0)]];
    uint out_var_INSTANCE [[user(locn1)]];
    float4 gl_Position [[position]];
};

vertex vsMain_out vsMain(constant type_ConstantBuffer_FrameConstants& frame [[buffer(0)]], const device type_StructuredBuffer_GpuInstance& instances [[buffer(1)]], uint gl_VertexIndex [[vertex_id]], uint gl_InstanceIndex [[instance_id]])
{
    vsMain_out out = {};
    uint _59 = frame.instanceBase + gl_InstanceIndex;
    float _96;
    do
    {
        bool _79;
        if (instances._m0[_59].kind != 3u)
        {
            _79 = instances._m0[_59].kind == 4u;
        }
        else
        {
            _79 = true;
        }
        if (_79)
        {
            _96 = 0.0;
            break;
        }
        if (instances._m0[_59].kind == 1u)
        {
            _96 = (((3.0 * instances._m0[_59].param) + abs(instances._m0[_59].extra.z)) + precise::max(abs(instances._m0[_59].extra.x), abs(instances._m0[_59].extra.y))) + 1.0;
            break;
        }
        _96 = 1.0;
        break;
    } while(false);
    float2 _110 = (instances._m0[_59].rect.xy - float2(_96)) + (float2(float(gl_VertexIndex & 1u), float(gl_VertexIndex >> 1u)) * (instances._m0[_59].rect.zw + float2(2.0 * _96)));
    float _112 = _110.x;
    float _115 = _110.y;
    out.gl_Position = float4((((float2((instances._m0[_59].transform.x * _112) + (instances._m0[_59].transform.z * _115), (instances._m0[_59].transform.y * _112) + (instances._m0[_59].transform.w * _115)) + instances._m0[_59].translate) * frame.inverseViewport) * float2(2.0, -2.0)) + float2(-1.0, 1.0), 0.0, 1.0);
    out.out_var_LOCAL = _110;
    out.out_var_INSTANCE = _59;
    return out;
}

