#include "linux_tests.hpp"

namespace linux_tests
{
    // Небольшая сцена без зависимости от установленных шрифтов.
    // A small scene independent of installed fonts.
    Scene::Scene(gran::rhi::RenderBackend& backend)
    {
        atlas = backend.createTexture({1, 1, gran::rhi::TextureFormat::R8, "test.atlas"});
        ramp = backend.createTexture({1, 1, gran::rhi::TextureFormat::RGBA8, "test.ramp"});
        std::uint8_t white[] = {255, 255, 255, 255};
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

    // Чтение проверяет и размер, и порядок RGBA-байтов, и верхнее начало координат.
    // Readback checks size, RGBA byte order and the top-down coordinate origin.
    void compare(gran::rhi::RenderBackend& backend, unsigned x, unsigned y, std::uint32_t color)
    {
        std::vector<std::uint8_t> rgba;
        unsigned width = 0; unsigned height = 0;
        dynamic_cast<gran::rhi::SnapshotSource&>(backend).readPixels(rgba, width, height);
        require(x < width && y < height && rgba.size() == static_cast<std::size_t>(width) * height * 4, "readback dimensions");
        std::size_t offset = (static_cast<std::size_t>(y) * width + x) * 4;
        for (unsigned channel = 0; channel < 4; ++channel)
            require(rgba[offset + channel] == ((color >> (8 * channel)) & 255), "readback color/orientation");
    }
}
