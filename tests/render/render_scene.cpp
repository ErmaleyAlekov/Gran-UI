#include "render_tests.hpp"

#include "gran/layout/layout.hpp"
#include "gran/markup/theme.hpp"
#include "gran/render/renderer.hpp"
#include "gran/rhi/backend_factory.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace render_tests
{
    // Реальный конвейер от XML до пикселей, отладочный слой проверяется после чтения GPU.
    // The real pipeline from XML to pixels; the debug layer is checked after GPU readback.
    gran::image::Image render(const Case& test, const Options& options)
    {
        std::string path = std::string("tests/render/scenes/") + test.name + ".xml";
        auto document = gran::markup::loadMarkupFile(path, gran::markup::LoadOptions{});
        const auto& theme = gran::markup::defaultTheme();
        auto window = gran::markup::windowStyle(theme, document.window);
        auto backend = gran::rhi::createPlatformBackend(gran::rhi::BackendOptions{nullptr, nullptr,
            static_cast<unsigned>(document.window.width * test.scale),
            static_cast<unsigned>(document.window.height * test.scale), false, true, options.backend, options.warp});
        gran::text::FontLibrary fonts;
        gran::render::Renderer renderer(*backend, fonts, document.baseDirectory);
        gran::ui::resolveStyles(*document.root, window, theme.kinds);
        gran::layout::layoutTree(*document.root, gran::Size{document.window.width, document.window.height}, renderer.measurer());
        if (renderer.render(*document.root, window, test.scale) != gran::rhi::FrameStatus::PRESENTED)
            throw gran::GpuException(gran::ErrorCode::GPU, "snapshot", 0);
        gran::image::Image pixels;
        dynamic_cast<gran::rhi::SnapshotSource&>(*backend).readPixels(pixels.pixels, pixels.width, pixels.height);
        auto messages = backend->takeDiagnostics();
        if (!messages.empty())
            throw gran::GpuException(gran::ErrorCode::GPU,
                gran::i18n::format(gran::i18n::Message::GPU_DEBUG_MESSAGE, {"render-tests", messages.front()}), 0);
        return pixels;
    }
}
