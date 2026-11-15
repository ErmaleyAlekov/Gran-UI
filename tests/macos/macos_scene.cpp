#include "macos_tests.hpp"

#include <vector>

namespace macos_tests
{
    void require(bool condition, const char* message)
    {
        if (!condition)
            throw std::runtime_error(message);
    }

    // Атлас и градиенты 1×1 белые; один экземпляр-прямоугольник, одно отсечение на весь кадр
    // The 1×1 atlas and ramps are white; one rectangle instance, one clip over the whole frame
    Scene::Scene(gran::rhi::RenderBackend& backend)
    {
        std::uint8_t white[] = {255, 255, 255, 255};

        atlas = backend.createTexture({1, 1, gran::rhi::TextureFormat::R8, "test.atlas"});
        ramp = backend.createTexture({1, 1, gran::rhi::TextureFormat::RGBA8, "test.ramp"});
        backend.updateTexture(atlas, {0, 0, 1, 1}, white, 1);
        backend.updateTexture(ramp, {0, 0, 1, 1}, white, 4);
        item.rect[0] = 4; item.rect[1] = 4; item.rect[2] = 16; item.rect[3] = 16;
        item.transform[0] = 1; item.transform[3] = 1;
        item.opacity = 1; item.fill = 0xff0000ff;
        segment.count = 1;
        frame.instances = &item; frame.instanceCount = 1; frame.clips = &clip; frame.clipCount = 1;
        frame.segments = &segment; frame.segmentCount = 1;
        frame.atlasSlot = atlas; frame.rampSlot = ramp;
        frame.clearColor[0] = 0; frame.clearColor[1] = 0; frame.clearColor[2] = 1;
    }

    // Пиксель сверяется точно: сплошная заливка внутри квадрата и фон не зависят от сглаживания
    // The pixel is compared exactly: the solid fill inside the square and the background do not depend on anti-aliasing
    void compare(gran::rhi::RenderBackend& backend, unsigned x, unsigned y, std::uint32_t color)
    {
        std::vector<std::uint8_t> rgba;
        unsigned width = 0;
        unsigned height = 0;

        dynamic_cast<gran::rhi::SnapshotSource&>(backend).readPixels(rgba, width, height);
        require(x < width && y < height && rgba.size() == static_cast<std::size_t>(width) * height * 4, "readback dimensions");
        std::size_t offset = (static_cast<std::size_t>(y) * width + x) * 4;
        for (unsigned channel = 0; channel < 4; ++channel)
            require(rgba[offset + channel] == ((color >> (8 * channel)) & 255), "readback color/orientation");
    }
}
