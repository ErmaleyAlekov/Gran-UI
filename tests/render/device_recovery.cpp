#include "render_tests.hpp"

#include "gran/app/internal/app_state.hpp"
#include "gran/utils/file_system.hpp"

#include <algorithm>
#include <iostream>
#include <utility>

namespace render_tests
{
    namespace
    {
        // Одна отрисовка проходит на GPU, следующий кадр сообщает о потере устройства.
        // One frame runs on the GPU, and the next reports device loss.
        class LosingBackend final : public gran::rhi::RenderBackend
        {
        public:
            explicit LosingBackend(std::unique_ptr<RenderBackend> backend)
                : _backend(std::move(backend)) {}

            void resize(std::uint32_t width, std::uint32_t height) override { _backend->resize(width, height); }
            std::uint32_t createTexture(const gran::rhi::TextureDesc& desc) override
            {
                return _backend->createTexture(desc);
            }
            void updateTexture(std::uint32_t slot, const gran::rhi::TextureRegion& region,
                const std::uint8_t* pixels, std::uint32_t pitch) override
            {
                _backend->updateTexture(slot, region, pixels, pitch);
            }
            void releaseTexture(std::uint32_t slot) override { _backend->releaseTexture(slot); }
            std::vector<std::string> takeDiagnostics() override { return _backend->takeDiagnostics(); }
            gran::rhi::FrameStatus render(const gran::rhi::FrameData& frame) override
            {
                if (_presented)
                    return gran::rhi::FrameStatus::DEVICE_LOST;
                const auto status = _backend->render(frame);
                _presented = status == gran::rhi::FrameStatus::PRESENTED;
                return status;
            }
            RenderBackend& underlying() { return *_backend; }
            bool presented() const { return _presented; }

        private:
            std::unique_ptr<RenderBackend> _backend;
            bool _presented = false;
        };

        // Читаем настоящий буфер GPU и проверяем сообщения отладочного слоя.
        // Read the actual GPU buffer and check the debug layer messages.
        bool capture(gran::rhi::RenderBackend& backend, gran::image::Image& image)
        {
            auto* source = dynamic_cast<gran::rhi::SnapshotSource*>(&backend);
            if (!source)
                return false;
            source->readPixels(image.pixels, image.width, image.height);
            const auto diagnostics = backend.takeDiagnostics();
            for (const auto& diagnostic : diagnostics)
                std::cerr << diagnostic << '\n';
            return diagnostics.empty();
        }

        // Ищем отличные от фона пиксели отдельно в областях текста и картинки.
        // Look for pixels distinct from the background in the text and image regions independently.
        bool hasContent(const gran::image::Image& image)
        {
            const auto& pixels = image.pixels;
            if (image.width != 160 || image.height != 120
                || pixels.size() != static_cast<std::size_t>(image.width) * image.height * 4)
                return false;
            bool text = false;
            bool picture = false;
            for (std::size_t y = 10; y < 105; ++y)
            {
                for (std::size_t x = 12; x < 145; ++x)
                {
                    const auto* pixel = pixels.data() + (y * image.width + x) * 4;
                    if (std::equal(pixels.data(), pixels.data() + 4, pixel)) continue;
                    if (y < 45) text = true;
                    if (y >= 60) picture = true;
                }
            }
            return text && picture;
        }

        // После кадра буферы выделены, а живая текстура учитывается до освобождения.
        // After a frame, buffers are allocated and a live texture is counted until release.
        bool tracksFrameMemory(gran::rhi::RenderBackend& backend)
        {
            const auto before = backend.videoMemoryInfo();
            const gran::rhi::TextureDesc desc{13, 7, gran::rhi::TextureFormat::RGBA8, "recovery-memory"};
            const auto slot = backend.createTexture(desc);
            const auto added = backend.videoMemoryInfo();
            backend.releaseTexture(slot);
            const auto removed = backend.videoMemoryInfo();
            const std::uint64_t expected = std::uint64_t(desc.width) * desc.height * 4;
            return before.bufferBytes > 0 && added.textureBytes == before.textureBytes + expected
                && removed.textureBytes == before.textureBytes;
        }

        // Повторяем штатный проход кадра после замены бэкенда и рендерера.
        // Repeat the normal frame pass after replacing the backend and renderer.
        bool recoversFrame(gran::app::AppWindow& win)
        {
            win.renderer.reset();
            auto wrapper = std::make_unique<LosingBackend>(std::move(win.backend));
            auto& failing = *wrapper;
            win.backend = std::move(wrapper);
            win.renderer = std::make_unique<gran::render::Renderer>(*win.backend, win.shared.fonts,
                win.document.baseDirectory);
            gran::image::Image before;
            gran::image::Image after;
            auto* root = win.document.root.get();
            gran::app::renderFrame(win);
            if (!dynamic_cast<LosingBackend*>(win.backend.get()) || !failing.presented()
                || !tracksFrameMemory(failing.underlying()) || !capture(failing.underlying(), before)
                || !hasContent(before))
                return false;
            win.changed = false;
            gran::app::renderFrame(win);
            if (dynamic_cast<LosingBackend*>(win.backend.get()) || win.deviceLost || !win.changed
                || win.document.root.get() != root)
                return false;
            gran::app::renderFrame(win);
            return !win.deviceLost && tracksFrameMemory(*win.backend) && capture(*win.backend, after)
                && before.width == after.width
                && before.height == after.height && before.pixels == after.pixels;
        }
    }

    // Окно без платформенного дескриптора использует выбранный GPU-бэкенд.
    // A window without a platform handle uses the selected GPU backend.
    bool runDeviceRecovery(const Options& options)
    {
        gran::Application::State state;
        state.options.headless = true;
        state.options.backend = options.backend;
        state.options.forceWarp = options.warp;
        state.options.debugLayer = true;
        gran::markup::LoadOptions markupOptions;
        markupOptions.baseDirectory = gran::fs::joinPath(gran::fs::parentPath(__FILE__), "../../samples/assets");
        auto document = gran::markup::loadMarkupString(R"xml(
            <Window width="160" height="120" background="#172033" color="#ffffff" fontSize="22">
                <Canvas>
                    <Text left="12" top="10">Gran GPU</Text>
                    <Image left="12" top="60" width="45" height="45" source="orb.png"/>
                </Canvas>
            </Window>
        )xml", markupOptions);
        auto& win = gran::app::addWindow(state, std::move(document), markupOptions.baseDirectory);
        gran::app::createGraphics(win);
        const bool passed = recoversFrame(win);
        if (passed) std::cout << "[ OK ] device recovery reproduces frame\n";
        else std::cout << "[FAIL] device recovery reproduces frame\n";
        return passed;
    }
}
