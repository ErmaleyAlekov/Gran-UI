#include "render_tests.hpp"
#include "gran/rhi/backend_factory.hpp"
#include "gran/core/gran_exception.hpp"

#include <iostream>

namespace render_tests
{
    namespace
    {
        // Невозможный размер отклоняется до чтения массива CPU из одного элемента.
        // An impossible count is rejected before reading a CPU array containing one element.
        bool rejectsOversizedFrame(gran::rhi::RenderBackend& backend)
        {
            gran::render::GpuInstance instance{};
            gran::render::GpuClip clip{};
            gran::rhi::FrameData frame;

            frame.instances = &instance;
            frame.instanceCount = 0xffffffffu;
            frame.clips = &clip;
            frame.clipCount = 1;
            try { backend.render(frame); }
            catch (const gran::GpuException& error)
            {
                return error.getCode() == gran::ErrorCode::INVALID_ARGUMENT;
            }
            return false;
        }

        // Стандартный экземпляр картинки без преобразования и отсечения.
        // A standard image instance without transformation or clipping.
        gran::render::GpuInstance image(float x, unsigned slot)
        {
            gran::render::GpuInstance item{};
            item.rect[0] = x;
            item.rect[2] = 8;
            item.rect[3] = 8;
            item.transform[0] = item.transform[3] = item.opacity = 1;
            item.extra[2] = item.extra[3] = 1;
            item.kind = gran::render::KIND_IMAGE;
            item.paint[0] = slot;
            return item;
        }

        // Три картинки A/B/A выявляют перестановку пакетов и неверную привязку текстур.
        // Three images A/B/A expose batch reordering and incorrect texture bindings.
        bool draw(gran::rhi::RenderBackend& backend)
        {
            gran::rhi::TextureDesc desc{1, 1};
            unsigned atlas = backend.createTexture(desc);
            unsigned ramp = backend.createTexture(desc);
            unsigned red = backend.createTexture(desc);
            unsigned green = backend.createTexture(desc);
            const std::uint8_t pixels[] = {255, 0, 0, 255, 0, 255, 0, 255};
            for (unsigned slot : {atlas, ramp, red, green})
                backend.updateTexture(slot, {0, 0, 1, 1}, pixels, 4);
            backend.updateTexture(green, {0, 0, 1, 1}, pixels + 4, 4);
            gran::render::GpuInstance instances[] = {image(0, red), image(8, green), image(16, red)};
            gran::render::GpuClip clip{};
            gran::rhi::FrameSegment segment{0, 3};
            gran::rhi::FrameData frame{instances, 3, &clip, 1, &segment, 1};
            frame.atlasSlot = atlas;
            frame.rampSlot = ramp;
            if (backend.render(frame) != gran::rhi::FrameStatus::PRESENTED) return false;
            gran::image::Image result;
            dynamic_cast<gran::rhi::SnapshotSource&>(backend).readPixels(result.pixels, result.width, result.height);
            bool passed = result.pixels[0] == 255 && result.pixels[8 * 4 + 1] == 255 && result.pixels[16 * 4] == 255;
            for (unsigned slot : {atlas, ramp, red, green}) backend.releaseTexture(slot);
            return passed;
        }
    }

    // После resize повторяем загрузку, рисование и освобождение слотов; слой должен молчать.
    // After resize repeat upload, drawing and slot release; the debug layer must stay quiet.
    bool runBackend(const Options& options)
    {
        auto backend = gran::rhi::createPlatformBackend(gran::rhi::BackendOptions{
            nullptr, nullptr, 32, 16, false, true, options.backend, options.warp});
        bool passed = draw(*backend);
        if (options.backend == gran::rhi::BackendKind::D3D11)
            passed = rejectsOversizedFrame(*backend) && passed;
        backend->resize(48, 20);
        passed = draw(*backend) && passed;
        auto messages = backend->takeDiagnostics();
        for (const auto& message : messages) std::cerr << message << '\n';
        passed = passed && messages.empty();
        if (passed) std::cout << "[ OK ] backend batching/resize/release\n";
        else std::cout << "[FAIL] backend batching/resize/release\n";
        return passed;
    }
}
