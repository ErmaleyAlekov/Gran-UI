#include "linux_tests.hpp"

namespace linux_tests
{
    // Слот заменяется при двух отправленных кадрах, пока старые дескрипторы ещё могут читаться GPU.
    // Replace a slot with two frames submitted while the GPU may still read its old descriptors.
    void textures(gran::rhi::RenderBackend& backend, Scene& scene)
    {
        scene.segment.backdropBefore = false;
        scene.item.kind = gran::render::KIND_IMAGE;
        scene.item.extra[2] = 1; scene.item.extra[3] = 1;
        unsigned slot = backend.createTexture({1, 1, gran::rhi::TextureFormat::RGBA8, "old.red"});
        scene.item.paint[0] = slot;
        std::uint8_t red[] = {255, 0, 0, 255}; std::uint8_t blue[] = {0, 0, 255, 255};
        backend.updateTexture(slot, {0, 0, 1, 1}, red, 4);
        backend.render(scene.frame); backend.render(scene.frame);
        backend.releaseTexture(slot);
        unsigned replacement = backend.createTexture({1, 1, gran::rhi::TextureFormat::RGBA8, "new.blue"});
        require(replacement == slot, "reused logical slot");
        scene.item.paint[0] = replacement;
        backend.updateTexture(replacement, {0, 0, 1, 1}, blue, 4);
        backend.render(scene.frame); backend.render(scene.frame);
        compare(backend, 8, 8, 0xffff0000);
        backend.releaseTexture(replacement);
    }
}
