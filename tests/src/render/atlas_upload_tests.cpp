#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "support/font_samples.hpp"
#include "gran/layout/layout.hpp"
#include "gran/markup/markup_loader.hpp"
#include "gran/markup/theme.hpp"
#include "gran/render/renderer.hpp"
#include "gran/render/gradient_ramps.hpp"
#include "gran/rhi/null/null_backend.hpp"
#include "gran/render/internal/scene_internal.hpp"
#include "gran/utils/mapped_file.hpp"

#include <stdexcept>
#include <utility>

namespace
{
    class FailingAtlasBackend : public gran::rhi::RenderBackend
    {
    public:
        void resize(std::uint32_t width, std::uint32_t height) override { backend.resize(width, height); }
        std::uint32_t createTexture(const gran::rhi::TextureDesc& desc) override
        {
            const auto slot = backend.createTexture(desc);
            if (desc.format == gran::rhi::TextureFormat::R8)
                atlas = slot;
            return slot;
        }

        void updateTexture(std::uint32_t slot, const gran::rhi::TextureRegion& region,
            const std::uint8_t* pixels, std::uint32_t rowPitch) override
        {
            if (slot == atlas)
            {
                ++atlasUploads;
                if (std::exchange(failNextAtlasUpload, false))
                    throw std::runtime_error("atlas upload failed");
            }
            backend.updateTexture(slot, region, pixels, rowPitch);
        }

        void releaseTexture(std::uint32_t slot) override { backend.releaseTexture(slot); }
        gran::rhi::FrameStatus render(const gran::rhi::FrameData& frame) override { return backend.render(frame); }
        std::vector<std::string> takeDiagnostics() override { return backend.takeDiagnostics(); }

        gran::rhi::NullBackend backend;
        std::uint32_t atlas = gran::render::NO_TEXTURE;
        unsigned atlasUploads = 0;
        bool failNextAtlasUpload = false;
    };

    void renderText(gran::render::Renderer& renderer, const char* markup)
    {
        auto document = gran::markup::loadMarkupString(markup, {});
        const auto& theme = gran::markup::defaultTheme();
        auto window = gran::markup::windowStyle(theme, document.window);

        gran::ui::resolveStyles(*document.root, window, theme.kinds);
        gran::layout::layoutTree(*document.root, {320, 200}, renderer.measurer());
        TEST_ASSERT(renderer.render(*document.root, window, 1.0f) == gran::rhi::FrameStatus::PRESENTED);
    }

    void testUnrepresentableGlyphSizes()
    {
        auto bytes = font_samples::buildFont({font_samples::squareGlyph()});
        auto path = font_samples::writeTemp(bytes, "glyph-paint-size.ttf");
        auto font = gran::text::FontFace::load(gran::MappedFile::open(path), 0, path);
        gran::rhi::NullBackend backend;
        gran::text::FontLibrary fonts;
        gran::render::TextEngine text(fonts);
        gran::text::GlyphAtlas atlas;
        gran::render::GradientRamps ramps;
        gran::render::ImageStore images(backend, {});
        gran::render::SceneResources resources{text, atlas, ramps, images};
        gran::render::Scene scene;
        gran::render::internal::SceneContext context{resources, scene, 1.0f, nullptr, {}};
        gran::text::TextLayout layout;
        layout.glyphs.push_back({font.get(), 0});

        // Оба выхода за диапазон раньше превращались в 40 px.
        // Both out-of-range sizes previously wrapped to 40 px.
        for (float size : {16424.0f, -16344.0f})
            gran::render::internal::paintGlyphs(layout, {}, {{}, size}, {}, context);
        TEST_ASSERT(scene.instances.empty());
        gran::render::internal::paintGlyphs(layout, {}, {{}, 40.0f}, {}, context);
        TEST_ASSERT(scene.instances.size() == 1);
    }

    void testPinnedGradientRows()
    {
        gran::render::GradientRamps ramps(1);
        gran::style::Gradient first;
        first.stops = {{{1, 0, 0, 1}, 0}, {{0, 1, 0, 1}, 1}};
        auto second = first;
        second.stops[0].color = {0, 0, 1, 1};

        TEST_ASSERT(ramps.rowFor(first) == 0);
        const auto generation = ramps.generation();
        TEST_ASSERT(ramps.rowFor(second, true) == ramps.capacity());
        TEST_ASSERT(ramps.capacity() == 1 && ramps.generation() == generation);
        TEST_ASSERT(ramps.rowFor(first, true) == 0);
        TEST_ASSERT(ramps.pixels()[0] == 255 && ramps.pixels()[2] == 0);
        TEST_ASSERT(ramps.rowFor(second) == 1);
        TEST_ASSERT(ramps.capacity() == 2 && ramps.generation() != generation);
    }

    void testAtlasUploadRetry()
    {
        FailingAtlasBackend backend;
        gran::text::FontLibrary fonts;
        gran::render::Renderer renderer(backend, fonts, {});
        const auto* next = "<Window><Text fontSize=\"40\">Z</Text></Window>";
        bool failed = false;

        backend.resize(320, 200);
        renderText(renderer, "<Window><Text fontSize=\"40\">A</Text></Window>");
        backend.failNextAtlasUpload = true;
        try { renderText(renderer, next); }
        catch (const std::runtime_error&) { failed = !backend.failNextAtlasUpload; }
        TEST_ASSERT(failed);
        const unsigned uploads = backend.atlasUploads;
        renderText(renderer, next);
        TEST_ASSERT(backend.atlasUploads == uploads + 1);
    }
}

namespace suites
{
    void registerAtlasUploadTests(test::TestRunner& runner)
    {
        runner.add("renderer: retries failed atlas upload", testAtlasUploadRetry);
        runner.add("renderer: keeps gradient rows stable during replay", testPinnedGradientRows);
        runner.add("renderer: skips glyph sizes outside the raster key range", testUnrepresentableGlyphSizes);
    }
}
