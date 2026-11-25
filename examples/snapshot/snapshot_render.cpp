#include "snapshot.hpp"

#include "gran/layout/layout.hpp"
#include "gran/markup/theme.hpp"
#include "gran/render/renderer.hpp"
#include "gran/rhi/backend_factory.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/utils/png_writer.hpp"

#include "gran/utils/file_system.hpp"

#include <chrono>
#include <iostream>

namespace snapshot
{
    namespace
    {
        // Миллисекунды с момента start / Milliseconds since start
        double elapsed(std::chrono::steady_clock::time_point start)
        {
            return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        }

        // Размер кадра: из аргументов, иначе из <Window> / The frame size: from the arguments, otherwise from <Window>
        gran::Size frameSize(const Arguments& arguments, const gran::markup::WindowSpec& window)
        {
            gran::Size size{window.width, window.height};

            if (arguments.width > 0.0f && arguments.height > 0.0f)
                size = gran::Size{arguments.width, arguments.height};
            return size;
        }

        // Шрифты <Font> относительно каталога разметки / The <Font> fonts relative to the markup directory
        void registerFonts(const gran::markup::UiDocument& document, const std::string& markup, gran::text::FontLibrary& fonts)
        {
            std::string base = gran::fs::parentPath(markup);

            for (const gran::markup::FontFace& face : document.fonts)
                fonts.addFile(gran::fs::joinPath(base, face.source));
        }

        // Пиксели кадра в PNG и сообщения отладочного слоя / The frame pixels into a PNG and the debug layer messages
        void savePixels(gran::rhi::RenderBackend& backend, const std::string& path)
        {
            std::vector<std::uint8_t> pixels;
            std::uint32_t width = 0;
            std::uint32_t height = 0;

            dynamic_cast<gran::rhi::SnapshotSource&>(backend).readPixels(pixels, width, height);
            if (!gran::writePng(path, pixels.data(), pixels.size(), width, height))
                throw gran::GranException(gran::ErrorCode::IO,
                    gran::i18n::format(gran::i18n::Message::IO_WRITE_FAILED, {path}));
            auto messages = backend.takeDiagnostics();
            if (!messages.empty())
                throw gran::GpuException(gran::ErrorCode::GPU,
                    gran::i18n::format(gran::i18n::Message::GPU_DEBUG_MESSAGE, {"snapshot", messages.front()}), 0);
        }
    }

    // Конвейер одного кадра с замерами: разметка+схема, стили+компоновка, отрисовка
    // One frame pipeline with timings: markup+schema, styles+layout, rendering
    int renderSnapshot(const Arguments& arguments)
    {
        auto start = std::chrono::steady_clock::now();
        gran::markup::UiDocument document = gran::markup::loadMarkupFile(arguments.markup, gran::markup::LoadOptions{});
        double loaded = elapsed(start);
        const gran::markup::Theme& theme = gran::markup::defaultTheme();
        gran::style::ComputedStyle window = gran::markup::windowStyle(theme, document.window);
        gran::Size size = frameSize(arguments, document.window);
        const double width = static_cast<double>(size.width) * arguments.scale;
        const double height = static_cast<double>(size.height) * arguments.scale;
        if (!(width >= 1 && width <= 16384 && height >= 1 && height <= 16384))
            throw gran::GranException(gran::ErrorCode::INVALID_ARGUMENT,
                gran::i18n::format(gran::i18n::Message::STYLE_BAD_NUMBER,
                    {std::to_string(width) + " x " + std::to_string(height)}));
        gran::text::FontLibrary fonts;
        auto backend = gran::rhi::createPlatformBackend(gran::rhi::BackendOptions{nullptr, nullptr,
            static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height),
            false, true, arguments.backend, arguments.forceWarp});
        gran::render::Renderer renderer(*backend, fonts, document.baseDirectory);

        registerFonts(document, arguments.markup, fonts);
        start = std::chrono::steady_clock::now();
        layoutDocument(document, window, size, renderer.measurer());
        double laidOut = elapsed(start);
        start = std::chrono::steady_clock::now();
        renderer.render(*document.root, window, arguments.scale);
        double rendered = elapsed(start);
        savePixels(*backend, arguments.output);
        std::cout << "load+schema " << loaded << " ms, style+layout " << laidOut << " ms, scene+submit " << rendered
                  << " ms, nodes " << document.nodeCount << '\n';
        return 0;
    }
}
