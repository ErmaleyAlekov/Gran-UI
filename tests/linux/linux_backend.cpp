#include "linux_tests.hpp"
#include "gran/core/gran_exception.hpp"

namespace linux_tests
{
    // Необработанная ошибка GPU или слоя завершает тест ненулевым кодом.
    // An unhandled GPU or validation error makes the test fail.
    void backend()
    {
        gran::rhi::BackendOptions options;
        options.backend = gran::rhi::BackendKind::VULKAN; options.forceWarp = !hardwareMode();
        options.debugLayer = true; options.width = 64; options.height = 48;
        auto renderer = gran::rhi::createPlatformBackend(options);
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
        require(rejected, "Linux must reject explicit Direct3D");
    }

    // Огромный счётчик не должен читать короткий массив CPU или попасть в драйвер.
    // An enormous count must not read a short CPU array or reach the driver.
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
}
