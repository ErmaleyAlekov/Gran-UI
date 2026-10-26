// Общие объявления шейдеров Gran UI. Структуры обязаны совпадать с include/gran/render/gpu_types.hpp
// (смещения закреплены static_assert на стороне C++). Один исходник для D3D12 (DXIL) и Vulkan (SPIR-V).
// The common Gran UI shader declarations. The structures must match include/gran/render/gpu_types.hpp
// (the offsets are pinned by static_assert on the C++ side). One source for D3D12 (DXIL) and Vulkan (SPIR-V).

#ifndef GRAN_UI_COMMON_HLSLI
#define GRAN_UI_COMMON_HLSLI

// Виды экземпляров (InstanceKind) / The instance kinds (InstanceKind)
#define KIND_RECT 0
#define KIND_SHADOW 1
#define KIND_INSET 2
#define KIND_GLYPH 3
#define KIND_IMAGE 4
#define KIND_BACKDROP 5

// Виды заливки (PaintMode) / The fill kinds (PaintMode)
#define PAINT_SOLID 0
#define PAINT_LINEAR 1
#define PAINT_RADIAL 2
#define PAINT_CONIC 3

// Постоянный слот подложки; атлас и рампа — в константах кадра / The fixed backdrop slot; the atlas and the ramp are in the frame constants
#define TEXTURE_BACKDROP 2
#define RAMP_WIDTH 256.0

// Экземпляр, 128 байт / An instance, 128 bytes
struct GpuInstance
{
    float4 rect;        // 0:   x, y, ширина, высота / x, y, width, height
    float4 radii;       // 16:  лв, пв, пн, лн / tl, tr, br, bl
    float4 border;      // 32:  верх, право, низ, лево / top, right, bottom, left
    float4 transform;   // 48:  a, b, c, d
    float2 translate;   // 64:  tx, ty
    float opacity;      // 72
    float param;        // 76:  сигма тени / the shadow sigma
    uint kind;          // 80
    uint clip;          // 84
    uint fill;          // 88
    uint stroke;        // 92
    float4 extra;       // 96:  UV / точки градиента / смещение и растяжение тени / UV / gradient points / shadow offset and spread
    uint4 paint;        // 112: текстура, строка рампы, PaintMode / the texture, the ramp row, PaintMode
};

// Отсечение, 64 байта / A clip, 64 bytes
struct GpuClip
{
    float4 rect;
    float4 radii;
    float4 inverse;
    float2 inverseTranslate;
    float2 padding;
};

// Константы кадра, 12 × 32 бита / The frame constants, 12 × 32 bits
struct FrameConstants
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

// Атрибуты Vulkan действуют только при компиляции в SPIR-V (dxc -spirv определяет __spirv__)
// The Vulkan attributes apply only when compiling to SPIR-V (dxc -spirv defines __spirv__)
#ifdef __spirv__
#define VK_BINDING(binding, set) [[vk::binding(binding, set)]]
#define VK_PUSH_CONSTANT [[vk::push_constant]]
#else
#define VK_BINDING(binding, set)
#define VK_PUSH_CONSTANT
#endif

// Привязки: регистры D3D12 и наборы Vulkan записаны рядом / The bindings: D3D12 registers and Vulkan sets side by side
VK_PUSH_CONSTANT ConstantBuffer<FrameConstants> frame : register(b0, space0);
VK_BINDING(0, 0) StructuredBuffer<GpuInstance> instances : register(t0, space0);
VK_BINDING(1, 0) StructuredBuffer<GpuClip> clips : register(t1, space0);
VK_BINDING(2, 0) SamplerState linearSampler : register(s0, space0);
VK_BINDING(0, 1) Texture2D textures[] : register(t0, space1);

// Распаковка RGBA8 (R — младший байт, альфа предумножена) / Unpacking RGBA8 (R is the lowest byte, premultiplied alpha)
float4 unpackColor(uint packed)
{
    return float4(packed & 0xFF, (packed >> 8) & 0xFF, (packed >> 16) & 0xFF, packed >> 24) / 255.0;
}

// Радиус угла по четверти точки (ось y направлена вниз) / The corner radius by the point quadrant (the y axis points down)
float pickRadius(float2 p, float4 radii)
{
    if (p.x < 0.0)
    {
        if (p.y < 0.0)
            return radii.x;
        return radii.w;
    }
    if (p.y < 0.0)
        return radii.y;
    return radii.z;
}

// SDF скруглённого прямоугольника с раздельными радиусами (p — от центра) / The rounded rectangle SDF with separate radii (p from the center)
float roundRectDistance(float2 p, float2 halfSize, float4 radii)
{
    float radius = pickRadius(p, radii);
    float2 q = abs(p) - halfSize + radius;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - radius;
}

// SDF прямоугольника rect (x, y, w, h) в точке p / The SDF of the rectangle rect (x, y, w, h) at the point p
float rectDistance(float2 p, float4 rect, float4 radii)
{
    float2 halfSize = rect.zw * 0.5;
    return roundRectDistance(p - rect.xy - halfSize, halfSize, radii);
}

// Аналитическое сглаживание: ширина пикселя — производная расстояния / Analytic antialiasing: the pixel width is the distance derivative
float coverage(float distance)
{
    float width = max(fwidth(distance), 1e-4);
    return saturate(0.5 - distance / width);
}

#endif
