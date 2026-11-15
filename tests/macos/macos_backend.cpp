#include "macos_tests.hpp"
#include "gran/core/gran_exception.hpp"

#include <iostream>
#include <string>

namespace macos_tests
{
    // Шесть кадров подряд проходят через оба кадра в полёте; затем смена размера с подложкой, слоты текстур,
    // отказ на Direct3D и на окно (окна у Metal до M6 нет)
    // Six frames in a row go through both frames in flight; then a resize with a backdrop, the texture slots,
    // rejecting Direct3D and a window (Metal has no window until M6)
    void backend()
    {
        gran::rhi::BackendOptions options;
        options.backend = gran::rhi::BackendKind::METAL;
        options.debugLayer = true; options.width = 64; options.height = 48;
        auto renderer = gran::rhi::createPlatformBackend(options);
        std::cout << "Metal adapter: " << renderer->adapterInfo().name << '\n';
        require(renderer->adapterInfo().api == "Metal", "adapter api");
        Scene scene(*renderer);
        for (unsigned frame = 0; frame < 6; ++frame)
            require(renderer->render(scene.frame) == gran::rhi::FrameStatus::PRESENTED, "render status");
        compare(*renderer, 8, 8, 0xff0000ff); compare(*renderer, 40, 30, 0xffff0000);
        validate(*renderer, scene.frame);
        renderer->resize(32, 24);
        scene.segment.backdropBefore = true; scene.segment.backdropRadius = 12;
        require(renderer->render(scene.frame) == gran::rhi::FrameStatus::PRESENTED, "resize/blur");
        compare(*renderer, 8, 8, 0xff0000ff);
        textures(*renderer, scene);
        auto diagnostics = renderer->takeDiagnostics();
        if (!diagnostics.empty()) throw std::runtime_error(diagnostics.front());
        bool rejected = false;
        try { options.backend = gran::rhi::BackendKind::D3D12; gran::rhi::createPlatformBackend(options); }
        catch (const gran::GpuException&) { rejected = true; }
        require(rejected, "macOS must reject explicit Direct3D");
    }

    // Неверные данные отклоняются исключением до записи в GPU, бэкенд остаётся рабочим
    // Invalid data is rejected with an exception before the GPU write, the backend stays usable
    void validate(gran::rhi::RenderBackend& backend, gran::rhi::FrameData frame)
    {
        bool rejected = false;
        std::uint8_t white[] = {255, 255, 255, 255};

        try { backend.updateTexture(frame.rampSlot, {1, 0, 1, 1}, white, 4); }
        catch (const gran::GpuException&) { rejected = true; }
        require(rejected, "texture bounds");
        rejected = false;
        try { backend.releaseTexture(0xffffffffu); }
        catch (const gran::GpuException&) { rejected = true; }
        require(rejected, "texture slot");
        frame.instanceCount = 0xffffffffu;
        rejected = false;
        try { backend.render(frame); }
        catch (const gran::GpuException&) { rejected = true; }
        require(rejected, "frame capacity");
    }

    // Слот красной картинки освобождается и отдаётся синей: кадр в полёте не должен увидеть подмену
    // The red image slot is released and given to a blue one: a frame in flight must not see the swap
    void textures(gran::rhi::RenderBackend& backend, Scene& scene)
    {
        std::uint8_t red[] = {255, 0, 0, 255};
        std::uint8_t blue[] = {0, 0, 255, 255};

        scene.segment.backdropBefore = false;
        scene.item.kind = gran::render::KIND_IMAGE;
        scene.item.extra[2] = 1; scene.item.extra[3] = 1;
        unsigned slot = backend.createTexture({1, 1, gran::rhi::TextureFormat::RGBA8, "old.red"});
        scene.item.paint[0] = slot;
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
