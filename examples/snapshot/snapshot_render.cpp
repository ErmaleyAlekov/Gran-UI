#include "snapshot.hpp"

#include "gran/layout/layout.hpp"
#include "gran/markup/theme.hpp"
#include "gran/render/renderer.hpp"
#include "gran/rhi/dx12/dx12_backend.hpp"
#include "gran/utils/png_writer.hpp"

#include <chrono>
#include <filesystem>
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
            std::filesystem::path base = std::filesystem::path(markup).parent_path();

            for (const gran::markup::FontFace& face : document.fonts)
                fonts.addFile((base / face.source).string());
        }

        // Пиксели кадра в PNG и сообщения отладочного слоя / The frame pixels into a PNG and the debug layer messages
        void savePixels(gran::rhi::RenderBackend& backend, const std::string& path)
        {
            std::vector<std::uint8_t> pixels;
            std::uint32_t width = 0;
            std::uint32_t height = 0;

            dynamic_cast<gran::rhi::SnapshotSource&>(backend).readPixels(pixels, width, height);
            gran::writePng(path, pixels.data(), width, height);
            for (const std::string& message : backend.takeDiagnostics())
                std::cout << message << '\n';
        }
    }

    // Конвейер одного кадра с замерами: разметка+схема, стили+компоновка, отрисовка
    // One frame pipeline with timings: markup+schema, styles+layout, rendering
    int renderSnapshot(const Arguments& arguments)
    {
        auto start = std::chrono::steady_clock::now();
        gran::markup::UiDocument document = gran::markup::loadMarkupFile(arguments.markup, gran::markup::LoadOptions{});
        buildLists(document);
        double loaded = elapsed(start);
        const gran::markup::Theme& theme = gran::markup::defaultTheme();
        gran::style::ComputedStyle window = gran::markup::windowStyle(theme, document.window);
        gran::Size size = frameSize(arguments, document.window);
        gran::text::FontLibrary fonts;
        auto backend = gran::rhi::createDx12Backend(gran::rhi::BackendOptions{nullptr, nullptr,
            static_cast<std::uint32_t>(size.width * arguments.scale), static_cast<std::uint32_t>(size.height * arguments.scale),
            false, true});
        gran::render::Renderer renderer(*backend, fonts, document.baseDirectory);

        registerFonts(document, arguments.markup, fonts);
        start = std::chrono::steady_clock::now();
        gran::ui::resolveStyles(*document.root, window, theme.kinds);
        gran::layout::layoutTree(*document.root, size, renderer.measurer());
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
