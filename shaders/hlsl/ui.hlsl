// Шейдеры интерфейса: один instanced-вызов рисует все виды экземпляров (убер-шейдер по полю kind).
// The interface shaders: one instanced call draws all instance kinds (an uber shader by the kind field).

#include "ui_common.hlsli"
#include "ui_shadow.hlsli"

// Выход вершинного шейдера / The vertex shader output
struct VsOut
{
    float4 position : SV_Position;
    float2 local : LOCAL;
    nointerpolation uint instance : INSTANCE;
};

// Запас квадрата вокруг блока: 1 px на сглаживание, у тени — 3σ, смещение и растяжение
// The quad margin around the box: 1 px for antialiasing, for a shadow — 3σ, the offset and the spread
float quadMargin(GpuInstance item)
{
    if (item.kind == KIND_GLYPH || item.kind == KIND_IMAGE)
        return 0.0;
    if (item.kind == KIND_SHADOW)
        return 3.0 * item.param + abs(item.extra.z) + max(abs(item.extra.x), abs(item.extra.y)) + 1.0;
    return 1.0;
}

// Угол квадрата из номера вершины (полоса из 4 вершин) / The quad corner from the vertex number (a strip of 4 vertices)
VsOut vsMain(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
{
    VsOut output;
    // SV_InstanceID в D3D12 не включает начальный экземпляр, а в Vulkan включает — база всегда из констант
    // SV_InstanceID excludes the start instance in D3D12 but includes it in Vulkan — the base always comes from the constants
    uint index = frame.instanceBase + instanceId;
    GpuInstance item = instances[index];
    float margin = quadMargin(item);
    float2 corner = float2(vertexId & 1, vertexId >> 1);
    float2 local = item.rect.xy - margin + corner * (item.rect.zw + 2.0 * margin);
    float2 screen = float2(item.transform.x * local.x + item.transform.z * local.y,
        item.transform.y * local.x + item.transform.w * local.y) + item.translate;

    output.position = float4(screen * frame.inverseViewport * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    output.local = local;
    output.instance = index;
    return output;
}

// Цвет градиента из строки рампы (рампа предумножена: смешивание как в CSS) / The gradient color from the ramp row (the ramp is premultiplied: blending as in CSS)
float4 gradientColor(GpuInstance item, float2 p)
{
    float t = 0.0;
    if (item.paint.z == PAINT_LINEAR)
    {
        float2 axis = item.extra.zw - item.extra.xy;
        t = dot(p - item.extra.xy, axis) / max(dot(axis, axis), 1e-6);
    }
    else if (item.paint.z == PAINT_RADIAL)
        t = length((p - item.extra.xy) / max(item.extra.zw, 1e-3));
    else
    {
        float2 d = p - item.extra.xy;
        t = frac((atan2(d.x, -d.y) - item.extra.z) / 6.2831853);
    }
    float u = (saturate(t) * (RAMP_WIDTH - 1.0) + 0.5) / RAMP_WIDTH;
    float v = (item.paint.y + 0.5) / max(frame.rampRows, 1);
    return textures[NonUniformResourceIndex(frame.rampSlot)].SampleLevel(linearSampler, float2(u, v), 0.0);
}

// Прямоугольник: заливка внутри внутреннего контура, рамка между контурами / A rectangle: the fill inside the inner outline, the border between the outlines
float4 shadeRect(GpuInstance item, float2 p)
{
    float outer = coverage(rectDistance(p, item.rect, item.radii));
    float inner = outer;
    float4 fill = unpackColor(item.fill);
    if (any(item.border > 0.0))
    {
        float4 innerRect = float4(item.rect.xy + item.border.wx, item.rect.zw - item.border.wx - item.border.yz);
        float4 innerRadii = max(item.radii - float4(max(item.border.w, item.border.x), max(item.border.y, item.border.x),
            max(item.border.y, item.border.z), max(item.border.w, item.border.z)), 0.0);
        inner = min(outer, coverage(rectDistance(p, innerRect, innerRadii)));
    }
    if (item.paint.z != PAINT_SOLID)
        fill = gradientColor(item, p);
    return fill * inner + unpackColor(item.stroke) * saturate(outer - inner);
}

// Глиф: покрытие из атласа (R8) / A glyph: the coverage from the atlas (R8)
float4 shadeGlyph(GpuInstance item, float2 p)
{
    float2 uv = lerp(item.extra.xy, item.extra.zw, (p - item.rect.xy) / item.rect.zw);
    float alpha = textures[NonUniformResourceIndex(frame.atlasSlot)].SampleLevel(linearSampler, uv, 0.0).r;
    return unpackColor(item.fill) * alpha;
}

// Картинка с маской скруглённых углов / An image with a rounded corner mask
float4 shadeImage(GpuInstance item, float2 p)
{
    uint texture = min(item.paint.x, max(frame.textureCount, 1) - 1);
    float2 uv = lerp(item.extra.xy, item.extra.zw, (p - item.rect.xy) / item.rect.zw);
    float4 color = textures[NonUniformResourceIndex(texture)].SampleLevel(linearSampler, uv, 0.0);
    return color * coverage(rectDistance(p, item.rect, item.radii));
}

// Подложка: размытая копия кадра под блоком плюс тонировка / A backdrop: a blurred frame copy under the box plus a tint
float4 shadeBackdrop(GpuInstance item, float2 p, float2 screen)
{
    float4 blurred = textures[TEXTURE_BACKDROP].SampleLevel(linearSampler, screen * frame.inverseViewport, 0.0);
    float4 tint = unpackColor(item.fill);
    float mask = coverage(rectDistance(p, item.rect, item.radii));
    return (tint + blurred * (1.0 - tint.a)) * mask;
}

// Покрытие отсечения в координатах окна (индекс 0 — без отсечения) / The clip coverage in window coordinates (index 0 — no clip)
float clipCoverage(uint index, float2 screen)
{
    if (index == 0)
        return 1.0;
    GpuClip clip = clips[min(index, max(frame.clipCount, 1) - 1)];
    float2 local = float2(clip.inverse.x * screen.x + clip.inverse.z * screen.y,
        clip.inverse.y * screen.x + clip.inverse.w * screen.y) + clip.inverseTranslate;
    return coverage(rectDistance(local, clip.rect, clip.radii));
}

// Пиксель: вид экземпляра выбирает функцию, затем прозрачность и отсечение / A pixel: the instance kind chooses the function, then the opacity and the clip
float4 psMain(VsOut input) : SV_Target
{
    GpuInstance item = instances[input.instance];
    float4 color;

    if (item.kind == KIND_RECT)
        color = shadeRect(item, input.local);
    else if (item.kind == KIND_SHADOW)
        color = shadeOuterShadow(item, input.local);
    else if (item.kind == KIND_INSET)
        color = shadeInsetShadow(item, input.local);
    else if (item.kind == KIND_GLYPH)
        color = shadeGlyph(item, input.local);
    else if (item.kind == KIND_IMAGE)
        color = shadeImage(item, input.local);
    else
        color = shadeBackdrop(item, input.local, input.position.xy);
    return color * item.opacity * clipCoverage(item.clip, input.position.xy);
}

// ---- Размытие подложки: двойной Kawase / The backdrop blur: dual Kawase ----
// Константы прохода размытия: viewport — размер цели, inverseViewport — размер текселя источника,
// instanceBase — индекс текстуры-источника. The blur pass constants: viewport is the target size,
// inverseViewport is the source texel size, instanceBase is the source texture index.

// Треугольник на весь экран без буфера вершин / A full screen triangle without a vertex buffer
float4 vsFullscreen(uint vertexId : SV_VertexID, out float2 uv : TEXCOORD0) : SV_Position
{
    uv = float2((vertexId << 1) & 2, vertexId & 2);
    return float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
}

// Уменьшение вдвое: центр с весом 4 и четыре диагонали / A downscale by half: the center with weight 4 and four diagonals
float4 psBlurDown(float4 position : SV_Position, float2 uv : TEXCOORD0) : SV_Target
{
    Texture2D source = textures[NonUniformResourceIndex(frame.instanceBase)];
    float2 o = frame.inverseViewport * 0.5;
    float4 sum = source.SampleLevel(linearSampler, uv, 0.0) * 4.0;
    sum += source.SampleLevel(linearSampler, uv - o, 0.0);
    sum += source.SampleLevel(linearSampler, uv + o, 0.0);
    sum += source.SampleLevel(linearSampler, uv + float2(o.x, -o.y), 0.0);
    sum += source.SampleLevel(linearSampler, uv - float2(o.x, -o.y), 0.0);
    return sum / 8.0;
}

// Увеличение вдвое: восемь отсчётов по кругу / An upscale by two: eight samples around a circle
float4 psBlurUp(float4 position : SV_Position, float2 uv : TEXCOORD0) : SV_Target
{
    Texture2D source = textures[NonUniformResourceIndex(frame.instanceBase)];
    float2 o = frame.inverseViewport * 0.5;
    float4 sum = source.SampleLevel(linearSampler, uv + float2(-o.x * 2.0, 0.0), 0.0);
    sum += source.SampleLevel(linearSampler, uv + float2(-o.x, o.y), 0.0) * 2.0;
    sum += source.SampleLevel(linearSampler, uv + float2(0.0, o.y * 2.0), 0.0);
    sum += source.SampleLevel(linearSampler, uv + float2(o.x, o.y), 0.0) * 2.0;
    sum += source.SampleLevel(linearSampler, uv + float2(o.x * 2.0, 0.0), 0.0);
    sum += source.SampleLevel(linearSampler, uv + float2(o.x, -o.y), 0.0) * 2.0;
    sum += source.SampleLevel(linearSampler, uv + float2(0.0, -o.y * 2.0), 0.0);
    sum += source.SampleLevel(linearSampler, uv + float2(-o.x, -o.y), 0.0) * 2.0;
    return sum / 12.0;
}
