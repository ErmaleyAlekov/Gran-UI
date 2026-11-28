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
#include "gran/text/glyph_rasterizer.hpp"
#include "gran/utils/mapped_file.hpp"

#include <stdexcept>
#include <utility>

namespace
{
    class FixtureRaster : public gran::text::RasterSource
    {
    public:
        bool rasterize(gran::text::GlyphBitmap& result) override
        {
            result = bitmap;
            return true;
        }

        gran::text::GlyphBitmap bitmap;
    };

    void testRasterBounds()
    {
        gran::text::GlyphAtlas atlas(16);
        gran::text::GlyphKey key{};
        FixtureRaster source;
        const auto originalGeneration = atlas.generation();
        for (const gran::text::GlyphBitmap& bitmap : {gran::text::GlyphBitmap{1, -1, 0, 0, {}},
            {2, 2, 0, 0, {255}}, {4097, 1, 0, 0, {}}, {1, 4097, 0, 0, {}},
            {1, 1, 40000, 0, {255}}, {1, 1, 0, -40000, {255}}})
        {
            source.bitmap = bitmap;
            TEST_ASSERT(atlas.raster(key, source) == nullptr);
            TEST_ASSERT(atlas.generation() == originalGeneration);
            ++key.shape;
        }
        source.bitmap = {1, 1, -32768, 32767, {255}};
        const auto* entry = atlas.raster(key, source);
        TEST_ASSERT(entry && entry->left == -32768 && entry->top == 32767);
    }

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
            if (std::exchange(failNextImageUpload, false))
                throw std::runtime_error("image upload failed");
            if (slot == atlas)
            {
                ++atlasUploads;
                if (std::exchange(failNextAtlasUpload, false))
                    throw std::runtime_error("atlas upload failed");
            }
            backend.updateTexture(slot, region, pixels, rowPitch);
        }

        void releaseTexture(std::uint32_t slot) override { backend.releaseTexture(slot); ++releases; }
        gran::rhi::FrameStatus render(const gran::rhi::FrameData& frame) override { return backend.render(frame); }
        std::vector<std::string> takeDiagnostics() override { return backend.takeDiagnostics(); }

        gran::rhi::NullBackend backend;
        std::uint32_t atlas = gran::render::NO_TEXTURE;
        unsigned atlasUploads = 0;
        bool failNextAtlasUpload = false;
        bool failNextImageUpload = false;
        unsigned releases = 0;
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

    void testExtremeControlGeometry()
    {
        gran::rhi::NullBackend backend;
        gran::text::FontLibrary fonts;
        gran::render::TextEngine text(fonts);
        gran::text::GlyphAtlas atlas;
        gran::render::GradientRamps ramps;
        gran::render::ImageStore images(backend, {});
        gran::render::SceneResources resources{text, atlas, ramps, images};
        gran::render::Scene scene;
        gran::render::internal::SceneContext context{resources, scene, 1.0f, nullptr, {}};
        gran::ui::Node node;
        node.layout.frame = {0, 0, 100, 20};
        node.computed.fontSize = 2;
        for (bool checked : {false, true})
        {
            node.content.checked = checked;
            gran::render::internal::paintSwitch(node, {}, context);
            TEST_ASSERT(scene.instances.size() == 3);
            const auto& track = scene.instances.front();
            const auto& knob = scene.instances.back();
            TEST_ASSERT(knob.rect[2] == 1 && knob.rect[3] == 1 && knob.radii[0] == 0.5f);
            TEST_ASSERT(knob.rect[0] >= track.rect[0] && knob.rect[0] + knob.rect[2] <= track.rect[0] + track.rect[2]);
            scene.instances.clear();
        }

        node.content.minimum = -3e38f;
        node.content.maximum = 3e38f;
        node.content.value = 0;
        node.kind = gran::ui::ElementKind::SLIDER;
        gran::render::internal::paintControl(node, {}, context);
        TEST_ASSERT(scene.instances.size() == 4);
        const auto& thumb = scene.instances.back();
        TEST_ASSERT(thumb.rect[0] + thumb.rect[2] * 0.5f == 50);
        scene.instances.clear();
        node.kind = gran::ui::ElementKind::PROGRESS_BAR;
        gran::render::internal::paintControl(node, {}, context);
        TEST_ASSERT(scene.instances.size() == 1 && scene.instances.front().rect[2] == 50);
    }

    void testEmptyImageBox()
    {
        gran::rhi::NullBackend backend;
        gran::text::FontLibrary fonts;
        gran::render::TextEngine text(fonts);
        gran::text::GlyphAtlas atlas;
        gran::render::GradientRamps ramps;
        gran::render::ImageStore images(backend, "samples/assets");
        gran::render::SceneResources resources{text, atlas, ramps, images};
        gran::render::Scene scene;
        gran::render::internal::SceneContext context{resources, scene, 1.0f, nullptr, {}};
        gran::ui::Node node;
        node.content.source = "orb.png";
        node.content.fit = gran::ui::ImageFit::CONTAIN;
        node.layout.frame = {0, 0, 0, 32};
        gran::render::internal::paintImage(node, {}, context);
        TEST_ASSERT(scene.instances.empty());
        node.layout.frame.width = 32;
        gran::render::internal::paintImage(node, {}, context);
        TEST_ASSERT(scene.instances.size() == 1);
    }

    void testDeepScene()
    {
        gran::rhi::NullBackend backend;
        gran::text::FontLibrary fonts;
        gran::render::TextEngine text(fonts);
        gran::text::GlyphAtlas atlas;
        gran::render::GradientRamps ramps;
        gran::render::ImageStore images(backend, "samples/assets");
        gran::render::SceneResources resources{text, atlas, ramps, images};
        gran::render::Scene scene;
        gran::ui::Node root;
        std::vector<gran::ui::Node*> chain{&root};
        for (unsigned depth = 0; depth < 512; ++depth)
            chain.push_back(&gran::ui::appendChild(*chain.back(), std::make_unique<gran::ui::Node>()));
        auto& leaf = *chain.back();
        leaf.kind = gran::ui::ElementKind::IMAGE;
        leaf.content.source = "orb.png";
        leaf.layout.frame = {0, 0, 32, 32};
        gran::render::buildScene(root, {}, 1, resources, scene);
        for (auto it = chain.rbegin(); it != chain.rend(); ++it)
            (*it)->children.clear();
        TEST_ASSERT(!scene.instances.empty() && scene.instances.back().kind == gran::render::KIND_IMAGE);
    }

    void testLayoutDropMarkGeometry()
    {
        using namespace gran;
        rhi::NullBackend backend;
        text::FontLibrary fonts;
        render::TextEngine text(fonts);
        text::GlyphAtlas atlas;
        render::GradientRamps ramps;
        render::ImageStore images(backend, "");
        render::SceneResources resources{text, atlas, ramps, images};
        render::Scene scene;
        ui::Node root;
        auto& target = ui::appendChild(root, std::make_unique<ui::Node>());
        target.layout.frame = {20, 30, 80, 40};
        resources.editDrop = &target;
        render::buildScene(root, {}, 1, resources, scene);
        TEST_ASSERT(scene.instances.size() == 1);
        const auto frame = scene.instances.back();
        TEST_ASSERT(frame.rect[0] == 20 && frame.rect[1] == 30 && frame.rect[2] == 80 && frame.rect[3] == 40);
        resources.dropEdge = render::DropEdge::BOTTOM;
        render::buildScene(root, {}, 1, resources, scene);
        TEST_ASSERT(scene.instances.size() == 1);
        const auto edge = scene.instances.back();
        TEST_ASSERT(edge.rect[0] == 20 && edge.rect[1] == 67 && edge.rect[2] == 80 && edge.rect[3] == 3);
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

    void testImageUploadRetry()
    {
        FailingAtlasBackend backend;
        {
            gran::render::ImageStore images(backend, "samples/assets");
            backend.failNextImageUpload = true;
            bool failed = false;
            try { images.get("orb.png"); }
            catch (const std::runtime_error&) { failed = !backend.failNextImageUpload; }
            TEST_ASSERT(failed && backend.releases == 1);
            const auto& entry = images.get("orb.png");
            TEST_ASSERT(!entry.failed && &entry == &images.get("./orb.png"));
        }
        TEST_ASSERT(backend.releases == 2);
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

    void testGradientCapacityLimits()
    {
        gran::render::GradientRamps minimum(0);
        gran::render::GradientRamps maximum(4097);
        gran::style::Gradient gradient;
        gradient.stops = {{{1, 0, 0, 1}, 0}};
        TEST_ASSERT(minimum.capacity() == 1 && minimum.rowFor(gradient) == 0);
        TEST_ASSERT(maximum.capacity() == 4096 && maximum.rowFor(gradient) == 0);
        TEST_ASSERT(minimum.pixels()[0] == 255 && maximum.pixels()[0] == 255);
    }

    void testCoincidentGradientStops()
    {
        gran::render::GradientRamps ramps(1);
        gran::style::Gradient gradient;
        constexpr float edge = 128.5f / 256.0f;
        gradient.stops = {{{0, 1, 0, 1}, 0}, {{0, 1, 0, 1}, edge},
            {{0, 0, 1, 1}, edge}, {{0, 0, 1, 1}, 1}};
        TEST_ASSERT(ramps.rowFor(gradient) == 0);
        const auto* pixels = ramps.pixels();
        TEST_ASSERT(pixels[127 * 4 + 1] == 255);
        TEST_ASSERT(pixels[128 * 4 + 2] == 255 && pixels[129 * 4 + 2] == 255);
        gradient.stops = {{{0, 1, 0, 1}, 0.5f / 256}, {{0, 0, 1, 1}, 0.5f / 256}};
        const auto row = ramps.rowFor(gradient);
        TEST_ASSERT(ramps.pixels()[row * 256 * 4 + 2] == 255);
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
        runner.add("renderer: atlas rejects invalid raster dimensions and bearings", testRasterBounds);
        runner.add("renderer: retries failed atlas upload", testAtlasUploadRetry);
        runner.add("renderer: skips an empty image box", testEmptyImageBox);
        runner.add("renderer: small switch and wide control ranges", testExtremeControlGeometry);
        runner.add("renderer: visits a deep programmatic scene", testDeepScene);
        runner.add("renderer: layout drop paints either the frame or its edge", testLayoutDropMarkGeometry);
        runner.add("renderer: releases failed image uploads and shares cached textures", testImageUploadRetry);
        runner.add("renderer: keeps gradient rows stable during replay", testPinnedGradientRows);
        runner.add("renderer: selects the last stop on a gradient edge", testCoincidentGradientStops);
        runner.add("renderer: clamps gradient ramp capacity before allocation", testGradientCapacityLimits);
        runner.add("renderer: skips glyph sizes outside the raster key range", testUnrepresentableGlyphSizes);
    }
}
