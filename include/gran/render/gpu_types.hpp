#ifndef GRAN_RENDER_GPU_TYPES_HPP
#define GRAN_RENDER_GPU_TYPES_HPP

#include <cstddef>
#include <cstdint>

namespace gran::render
{
    /*
    ** Структуры, общие для C++ и HLSL (shaders/hlsl/ui_common.hlsli). Только 4-байтовые
    ** поля, размер кратен 16, смещения закреплены static_assert: расхождение с шейдером
    ** ловится при сборке, а не испорченным кадром или сбросом драйвера.
    **
    ** The structures shared by C++ and HLSL (shaders/hlsl/ui_common.hlsli). Only 4-byte
    ** fields, the size is a multiple of 16, the offsets are pinned by static_assert: a mismatch
    ** with the shader is caught at build time, not by a broken frame or a driver reset.
    */

    // Вид экземпляра: что рисует пиксельный шейдер / The instance kind: what the pixel shader draws
    enum InstanceKind : std::uint32_t
    {
        KIND_RECT = 0,      // скруглённый прямоугольник: заливка + рамка / a rounded rectangle: fill + border
        KIND_SHADOW = 1,    // внешняя тень box-shadow / an outer box-shadow
        KIND_INSET = 2,     // внутренняя тень (inset) / an inner (inset) shadow
        KIND_GLYPH = 3,     // глиф из атласа покрытия / a glyph from the coverage atlas
        KIND_IMAGE = 4,     // картинка RGBA / an RGBA image
        KIND_BACKDROP = 5   // размытая подложка (glassmorphism) / a blurred backdrop (glassmorphism)
    };

    // Вид заливки прямоугольника (поле paint.z) / The rectangle fill kind (the paint.z field)
    enum PaintMode : std::uint32_t
    {
        PAINT_SOLID = 0,
        PAINT_LINEAR = 1,
        PAINT_RADIAL = 2,
        PAINT_CONIC = 3
    };

    /*
    ** Экземпляр: 8 слотов по 16 байт. Координаты — пиксели устройства в локальной системе
    ** блока; transform переводит их в координаты окна. Цвета — RGBA8 с предумноженной альфой.
    **
    ** An instance: 8 slots of 16 bytes. Coordinates are device pixels in the local box
    ** system; transform turns them into window coordinates. Colors are premultiplied RGBA8.
    */
    struct GpuInstance
    {
        float rect[4];          // x, y, ширина, высота / x, y, width, height
        float radii[4];         // радиусы: лв, пв, пн, лн / radii: tl, tr, br, bl
        float border[4];        // рамка: верх, право, низ, лево / border: top, right, bottom, left
        float transform[4];     // a, b, c, d (x' = a·x + c·y + tx)
        float translate[2];     // tx, ty
        float opacity;          // непрозрачность 0..1 / the opacity 0..1
        float param;            // сигма тени, сила подложки / the shadow sigma, the backdrop strength
        std::uint32_t kind;     // InstanceKind
        std::uint32_t clip;     // индекс в таблице отсечений (0 — без отсечения) / the clip table index (0 — no clip)
        std::uint32_t fill;     // цвет заливки / тени / глифа / the fill / shadow / glyph color
        std::uint32_t stroke;   // цвет рамки / the border color
        float extra[4];         // UV (глиф, картинка), точки градиента, смещение и растяжение тени / UV, gradient points, shadow offset and spread
        std::uint32_t paint[4]; // текстура, строка рампы, PaintMode, резерв / the texture, the ramp row, PaintMode, reserved
    };

    /*
    ** Отсечение скруглённым прямоугольником в локальных координатах; inverse переводит
    ** координаты окна в локальные (a, b, c, d, tx, ty).
    **
    ** A clip by a rounded rectangle in local coordinates; inverse turns window
    ** coordinates into local ones (a, b, c, d, tx, ty).
    */
    struct GpuClip
    {
        float rect[4];
        float radii[4];
        float inverse[4];
        float inverseTranslate[2];
        float padding[2];
    };

    // Константы кадра (корневые константы D3D12 / push-константы Vulkan) / The frame constants (D3D12 root constants / Vulkan push constants)
    struct FrameConstants
    {
        float viewport[2];          // размер цели в пикселях / the target size in pixels
        float inverseViewport[2];   // 1 / размер / 1 / size
        std::uint32_t instanceBase; // первый экземпляр сегмента / the first instance of the segment
        std::uint32_t clipCount;    // число отсечений (защита индекса) / the clip count (an index guard)
        std::uint32_t textureCount; // число текстур (защита индекса) / the texture count (an index guard)
        std::uint32_t rampRows;     // число строк рампы градиентов / the gradient ramp row count
        std::uint32_t atlasSlot;    // слот атласа глифов / the glyph atlas slot
        std::uint32_t rampSlot;     // слот рампы градиентов / the gradient ramp slot
        std::uint32_t reserved[2];  // выравнивание до 16 байт / padding to 16 bytes
    };

    static_assert(sizeof(GpuInstance) == 128, "GpuInstance должен совпадать с HLSL / must match HLSL");
    static_assert(offsetof(GpuInstance, radii) == 16, "смещение radii / the radii offset");
    static_assert(offsetof(GpuInstance, border) == 32, "смещение border / the border offset");
    static_assert(offsetof(GpuInstance, transform) == 48, "смещение transform / the transform offset");
    static_assert(offsetof(GpuInstance, translate) == 64, "смещение translate / the translate offset");
    static_assert(offsetof(GpuInstance, opacity) == 72, "смещение opacity / the opacity offset");
    static_assert(offsetof(GpuInstance, kind) == 80, "смещение kind / the kind offset");
    static_assert(offsetof(GpuInstance, extra) == 96, "смещение extra / the extra offset");
    static_assert(offsetof(GpuInstance, paint) == 112, "смещение paint / the paint offset");
    static_assert(sizeof(GpuClip) == 64, "GpuClip должен совпадать с HLSL / must match HLSL");
    static_assert(offsetof(GpuClip, inverse) == 32, "смещение inverse / the inverse offset");
    static_assert(sizeof(FrameConstants) == 48, "FrameConstants — 12 корневых констант / 12 root constants");
    static_assert(offsetof(FrameConstants, atlasSlot) == 32, "смещение atlasSlot / the atlasSlot offset");

    // Число 32-битных корневых констант / The number of 32-bit root constants
    constexpr std::uint32_t FRAME_CONSTANT_COUNT = sizeof(FrameConstants) / 4;

    // Ширина строки рампы градиента в текселях / The gradient ramp row width in texels
    constexpr std::uint32_t RAMP_WIDTH = 256;

    // Постоянный слот подложки (принадлежит бэкенду); атлас и рампа — в константах кадра
    // The fixed backdrop slot (owned by the backend); the atlas and the ramp are in the frame constants
    constexpr std::uint32_t TEXTURE_BACKDROP = 2;
}

#endif
