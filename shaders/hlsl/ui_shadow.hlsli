// Тени box-shadow: гауссово размытие скруглённого прямоугольника. По x интеграл берётся точно
// (через erf), по y — четырьмя отсчётами (метод Э. Уоллеса, «Fast Rounded Rectangle Shadows»).
// box-shadow shadows: a Gaussian blur of a rounded rectangle. Along x the integral is exact
// (through erf), along y it uses four samples (E. Wallace's method, "Fast Rounded Rectangle Shadows").

#ifndef GRAN_UI_SHADOW_HLSLI
#define GRAN_UI_SHADOW_HLSLI

// Плотность нормального распределения / The normal distribution density
float gaussian(float x, float sigma)
{
    return exp(-(x * x) / (2.0 * sigma * sigma)) / (2.5066283 * sigma);
}

// Приближение erf (Абрамовиц — Стиган, погрешность < 5·10⁻⁴) / The erf approximation (Abramowitz — Stegun, error < 5·10⁻⁴)
float2 erfApprox(float2 x)
{
    float2 s = sign(x);
    float2 a = abs(x);
    float2 t = 1.0 + (0.278393 + (0.230389 + 0.078108 * (a * a)) * a) * a;
    t *= t;
    return s - s / (t * t);
}

// Размытая по x полоса прямоугольника на высоте y с учётом скругления / The x-blurred rectangle strip at height y with the rounding
float shadowStrip(float x, float y, float sigma, float corner, float2 halfSize)
{
    float delta = min(halfSize.y - corner - abs(y), 0.0);
    float curved = halfSize.x - corner + sqrt(max(0.0, corner * corner - delta * delta));
    float2 integral = 0.5 + 0.5 * erfApprox((x + float2(-curved, curved)) * (0.70710678 / sigma));
    return integral.y - integral.x;
}

// Доля тени в точке p (центр и полуразмеры фигуры, сигма, радиус угла) / The shadow share at the point p
float blurredBox(float2 p, float2 center, float2 halfSize, float sigma, float corner)
{
    float2 q = p - center;
    float low = q.y - halfSize.y;
    float high = q.y + halfSize.y;
    float start = clamp(-3.0 * sigma, low, high);
    float end = clamp(3.0 * sigma, low, high);
    float step = (end - start) / 4.0;
    float y = start + step * 0.5;
    float value = 0.0;

    for (int i = 0; i < 4; ++i)
    {
        value += shadowStrip(q.x, q.y - y, sigma, corner, halfSize) * gaussian(y, sigma) * step;
        y += step;
    }
    return value;
}

// Фигура тени: блок со смещением и растяжением; радиус растёт на растяжение (как в CSS)
// The shadow shape: the box with the offset and the spread; the radius grows by the spread (as in CSS)
float shadowShape(GpuInstance item, float2 p, float spreadSign)
{
    float spread = item.extra.z * spreadSign;
    float2 halfSize = max(item.rect.zw * 0.5 + spread, 0.0);
    float2 center = item.rect.xy + item.rect.zw * 0.5 + item.extra.xy;
    float corner = max(pickRadius(p - center, item.radii) + spread, 0.0);
    corner = min(corner, min(halfSize.x, halfSize.y));
    if (item.param < 0.5)
        return coverage(roundRectDistance(p - center, halfSize, float4(corner, corner, corner, corner)));
    return blurredBox(p, center, halfSize, item.param, corner);
}

// Внешняя тень видна только снаружи блока (CSS обрезает её рамкой элемента)
// An outer shadow is visible only outside the box (CSS clips it by the element border box)
float4 shadeOuterShadow(GpuInstance item, float2 p)
{
    float hole = coverage(rectDistance(p, item.rect, item.radii));
    return unpackColor(item.fill) * shadowShape(item, p, 1.0) * (1.0 - hole);
}

// Внутренняя тень: внутри блока, вне сжатой на растяжение фигуры / An inset shadow: inside the box, outside the shape shrunk by the spread
float4 shadeInsetShadow(GpuInstance item, float2 p)
{
    float inside = coverage(rectDistance(p, item.rect, item.radii));
    return unpackColor(item.fill) * (1.0 - shadowShape(item, p, -1.0)) * inside;
}

#endif
