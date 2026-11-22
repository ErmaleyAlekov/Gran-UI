#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/input/input_router.hpp"
#include "gran/style/style_sheet.hpp"
#include "gran/text/glyph_atlas.hpp"
#include "gran/text/glyph_rasterizer.hpp"

#include <stdexcept>

namespace
{
    class EventSink : public gran::input::ActionSink
    {
    public:
        void action(const std::string&, gran::ui::Node&) override {}
    };

    class TestRaster : public gran::text::RasterSource
    {
    public:
        bool rasterize(gran::text::GlyphBitmap& bitmap) override
        {
            bitmap.width = 3;
            bitmap.height = 2;
            bitmap.coverage.assign(pixelCount, 255);
            return true;
        }
        unsigned pixelCount = 5;
    };

    void testInvalidGlyphBitmap()
    {
        gran::text::GlyphAtlas atlas;
        TestRaster source;
        const auto generation = atlas.generation();

        TEST_ASSERT(atlas.raster({}, source) == nullptr);
        TEST_ASSERT(atlas.generation() == generation);
    }

    void testStableGlyphAtlasLayout()
    {
        gran::text::GlyphAtlas atlas(4);
        TestRaster source;
        gran::text::GlyphKey first;
        gran::text::GlyphKey second;

        source.pixelCount = 6;
        second.shape = 1;
        const auto* initial = atlas.raster(first, source);
        TEST_ASSERT(initial != nullptr);
        const auto generation = atlas.generation();
        TEST_ASSERT(atlas.raster(second, source, true) == nullptr);
        TEST_ASSERT(atlas.raster(first, source, true) == initial);
        TEST_ASSERT(atlas.size() == 4 && atlas.generation() == generation);
        TEST_ASSERT(initial->x == 0 && initial->y == 0 && initial->width == 3 && initial->height == 2);
        TEST_ASSERT(atlas.pixels()[0] == 255 && atlas.pixels()[6] == 255);
        TEST_ASSERT(atlas.raster(second, source) != nullptr);
        TEST_ASSERT(atlas.size() == 8 && atlas.generation() != generation);
        TEST_ASSERT(atlas.pixels()[0] == 255 && atlas.pixels()[10] == 255);
    }

    void testCyclicStyle()
    {
        gran::style::StyleRule rule;
        rule.basedOn = &rule;
        gran::style::CascadeInput input;
        input.rule = &rule;
        bool rejected = false;

        try { gran::style::computeStyle(input); }
        catch (const gran::GranException& error)
        {
            rejected = error.getCode() == gran::ErrorCode::RESOURCE;
        }
        TEST_ASSERT(rejected);
    }

    void testCyclicEventPath()
    {
        gran::ui::Node first;
        gran::ui::Node second;
        EventSink sink;
        gran::ui::UiEvent event;

        first.parent = &second;
        second.parent = &first;
        event.target = &first;
        TEST_ASSERT(!gran::input::dispatchEvent(event, sink));
        TEST_ASSERT(event.handled == 0);
    }

    void testThrowingEventHandler()
    {
        gran::ui::Node target;
        EventSink sink;
        gran::ui::UiEvent event;
        bool propagated = false;

        event.target = &target;
        target.listeners.push_back({event.type, {}, false, 1, [](auto&, auto&)
        {
            throw std::runtime_error("handler failed");
        }});
        try { gran::input::dispatchEvent(event, sink); }
        catch (const std::runtime_error&) { propagated = true; }
        TEST_ASSERT(propagated);
        TEST_ASSERT(event.current == nullptr);
        TEST_ASSERT(event.phase == gran::ui::EventPhase::NONE);
    }
}

namespace suites
{
    void registerEarlyReviewTests(test::TestRunner& runner)
    {
        runner.add("glyph atlas: rejects incomplete bitmap", testInvalidGlyphBitmap);
        runner.add("glyph atlas: preserves raster coordinates during stable layout", testStableGlyphAtlasLayout);
        runner.add("style: rejects cyclic basedOn", testCyclicStyle);
        runner.add("events: reject cyclic parent path", testCyclicEventPath);
        runner.add("events: clear dispatch state on exception", testThrowingEventHandler);
    }
}
