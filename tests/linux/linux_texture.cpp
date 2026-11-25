#include "linux_tests.hpp"
#include "gran/core/gran_exception.hpp"

namespace linux_tests
{
    namespace
    {
        void rejectFrame(gran::rhi::RenderBackend& backend, const gran::rhi::FrameData& frame)
        {
            try { backend.render(frame); }
            catch (const gran::GpuException& error)
            {
                require(error.getCode() == gran::ErrorCode::GPU, "invalid texture rejected before rendering");
                return;
            }
            require(false, "invalid texture slot accepted");
        }
    }

    // Слот заменяется при двух отправленных кадрах, пока старые дескрипторы ещё могут читаться GPU.
    // Replace a slot with two frames submitted while the GPU may still read its old descriptors.
    void textures(gran::rhi::RenderBackend& backend, Scene& scene)
    {
        scene.segment.backdropBefore = false;
        scene.item.kind = gran::render::KIND_IMAGE;
        scene.item.extra[2] = 1; scene.item.extra[3] = 1;
        unsigned slot = backend.createTexture({1, 1, gran::rhi::TextureFormat::RGBA8, "old.red"});
        scene.item.paint[0] = slot;
        auto invalid = scene.frame;
        invalid.atlasSlot = ~0u;
        rejectFrame(backend, invalid);
        invalid = scene.frame;
        invalid.rampSlot = ~0u;
        rejectFrame(backend, invalid);
        std::uint8_t red[] = {255, 0, 0, 255}; std::uint8_t blue[] = {0, 0, 255, 255};
        backend.updateTexture(slot, {0, 0, 1, 1}, red, 4);
        backend.render(scene.frame); backend.render(scene.frame);
        backend.releaseTexture(slot);
        rejectFrame(backend, scene.frame);
        invalid = scene.frame;
        invalid.atlasSlot = slot;
        rejectFrame(backend, invalid);
        invalid = scene.frame;
        invalid.rampSlot = slot;
        rejectFrame(backend, invalid);
        unsigned replacement = backend.createTexture({1, 1, gran::rhi::TextureFormat::RGBA8, "new.blue"});
        require(replacement == slot, "reused logical slot");
        scene.item.paint[0] = replacement;
        backend.updateTexture(replacement, {0, 0, 1, 1}, blue, 4);
        backend.render(scene.frame); backend.render(scene.frame);
        compare(backend, 8, 8, 0xffff0000);
        backend.releaseTexture(replacement);
    }
}
