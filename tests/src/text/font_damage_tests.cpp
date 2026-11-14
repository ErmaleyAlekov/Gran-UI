#include "framework/test_framework.hpp"
#include "gran/text/font_face.hpp"
#include "gran/utils/mapped_file.hpp"
#include "support/font_samples.hpp"
#include <chrono>

namespace
{
    void testFontDamageCompositeWhole()
    {
        auto g = font_samples::compositeHeader();
        font_samples::appendComponent(g, font_samples::ARGS_ARE_XY, 0);
        auto bytes = font_samples::buildFont({font_samples::squareGlyph(), g});
        auto path = font_samples::writeTemp(bytes, "composite_whole.ttf");
        auto font = gran::text::FontFace::load(gran::MappedFile::open(path), 0, path);
        gran::text::Outline result;
        TEST_ASSERT(font->outline(1, result));
        TEST_ASSERT(result.points.size() == 4);
    }

    void testFontDamageCompositeTruncated()
    {
        auto g = font_samples::compositeHeader();
        font_samples::appendComponent(g, font_samples::ARGS_ARE_XY | font_samples::ARGS_ARE_WORDS | font_samples::HAS_TWO_BY_TWO, 0);
        g.resize(14);
        auto bytes = font_samples::buildFont({font_samples::squareGlyph(), g, font_samples::squareGlyph()});
        auto path = font_samples::writeTemp(bytes, "composite_truncated.ttf");
        auto font = gran::text::FontFace::load(gran::MappedFile::open(path), 0, path);
        gran::text::Outline result;
        TEST_ASSERT(!font->outline(1, result));
    }

    void testFontDamageComponentOutOfRange()
    {
        auto g1 = font_samples::compositeHeader();
        font_samples::appendComponent(g1, font_samples::ARGS_ARE_XY, 7);
        auto bytes = font_samples::buildFont({font_samples::squareGlyph(), g1, font_samples::squareGlyph()});
        auto path = font_samples::writeTemp(bytes, "component_out_of_range.ttf");
        auto font = gran::text::FontFace::load(gran::MappedFile::open(path), 0, path);
        gran::text::Outline result;
        TEST_ASSERT(!font->outline(1, result));
    }

    void testFontDamageSimpleTruncated()
    {
        auto s = font_samples::squareGlyph();
        s.resize(12);
        auto bytes = font_samples::buildFont({font_samples::squareGlyph(), s, font_samples::squareGlyph()});
        auto path = font_samples::writeTemp(bytes, "simple_truncated.ttf");
        auto font = gran::text::FontFace::load(gran::MappedFile::open(path), 0, path);
        gran::text::Outline result;
        TEST_ASSERT(!font->outline(1, result));
        TEST_ASSERT(font->outline(2, result));
    }

    void testFontDamageCompositeLoop()
    {
        auto g = font_samples::compositeHeader();
        for (int i = 0; i < 300; ++i)
        {
            font_samples::appendComponent(g, font_samples::ARGS_ARE_XY | font_samples::MORE_COMPONENTS, 1);
        }
        font_samples::appendComponent(g, font_samples::ARGS_ARE_XY, 1);
        // Глиф 1 ссылается сам на себя 301 раз: без предела посещений это 301^8 вызовов
        // The glyph 1 refers to itself 301 times: without the visit limit that is 301^8 calls
        auto bytes = font_samples::buildFont({font_samples::squareGlyph(), g});
        auto path = font_samples::writeTemp(bytes, "composite_loop.ttf");
        auto font = gran::text::FontFace::load(gran::MappedFile::open(path), 0, path);
        gran::text::Outline result;
        auto start = std::chrono::steady_clock::now();
        TEST_ASSERT(!font->outline(1, result));
        auto end = std::chrono::steady_clock::now();
        TEST_ASSERT(std::chrono::duration_cast<std::chrono::seconds>(end - start) < std::chrono::seconds(2));
    }
}

namespace suites
{
    void registerFontDamageTests(test::TestRunner& runner)
    {
        runner.add("font damage: composite whole", testFontDamageCompositeWhole);
        runner.add("font damage: composite truncated", testFontDamageCompositeTruncated);
        runner.add("font damage: component out of range", testFontDamageComponentOutOfRange);
        runner.add("font damage: simple truncated", testFontDamageSimpleTruncated);
        runner.add("font damage: composite loop", testFontDamageCompositeLoop);
    }
}
