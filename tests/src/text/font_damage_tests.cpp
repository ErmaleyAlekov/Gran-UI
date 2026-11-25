#include "gran/text/internal/font_parts.hpp"
#include "framework/test_framework.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/render/internal/scene_internal.hpp"
#include "gran/text/internal/text_internal.hpp"
#include "gran/text/font_face.hpp"
#include "gran/text/font_library.hpp"
#include "gran/res/resources.hpp"
#include "gran/utils/mapped_file.hpp"
#include "support/font_samples.hpp"
#include <chrono>

namespace
{
    // Шрифт с именем семейства для проверки каталога.
    // A font with a family name for exercising the font catalog.
    font_samples::Bytes namedReviewFont()
    {
        const font_samples::Bytes name{0,0, 0,1, 0,18, 0,1, 0,0, 0,0, 0,1, 0,11, 0,0,
            'G','r','a','n',' ','R','e','v','i','e','w'};
        return font_samples::buildFont({font_samples::squareGlyph()}, {}, {}, name);
    }

    // Метрики раскладки фиксированы пробелом между буквами, независимо от системного шрифта.
    // Letter spacing fixes the layout widths independently of the system font.
    gran::text::TextStyle setupLayoutFont(gran::text::FontLibrary& fonts)
    {
        static const auto font = namedReviewFont();
        const gran::res::Entry entry{"font-catalog/layout.ttf", font.data(), font.size()};
        gran::res::mount({"font-layout-test", &entry, 1});
        fonts.addFile("res:/font-catalog/layout.ttf");
        gran::text::TextStyle style;
        style.family = "Gran Review";
        style.size = 0;
        style.letterSpacing = 1;
        style.wrap = true;
        return style;
    }

    void testTextRunBoundaries()
    {
        gran::text::FontLibrary fonts;
        auto style = setupLayoutFont(fonts);
        style.ellipsis = true;
        TEST_ASSERT(gran::text::layoutText("", style, -1, fonts).glyphs.empty());
        style.ellipsis = false;
        const std::vector<gran::text::TextRun> excessive(65536);
        TEST_ASSERT_THROWS(gran::text::layoutRuns("", excessive, style, 5, fonts), gran::GranException);
        const std::vector<gran::text::TextRun> runs{{0, 5, style}, {5, 7, style}};
        const auto hyphen = gran::text::layoutRuns("a co-op", runs, style, 5, fonts);
        TEST_ASSERT(hyphen.lines.size() == 2 && hyphen.lines[0].count == 5);

        const auto fitting = gran::text::layoutText(u8"x ab\u00a0cd", style, 5, fonts);
        const auto overlong = gran::text::layoutText(u8"x ab\u00a0cd", style, 4, fonts);
        TEST_ASSERT(fitting.lines.size() == 2 && fitting.lines[1].count == 5);
        TEST_ASSERT(overlong.lines.size() == 3);
        TEST_ASSERT(!gran::text::breakOpportunityAfter(U'中', U'\u00a0'));
        TEST_ASSERT(!gran::text::breakOpportunityAfter(U'\u00a0', U'中'));
    }

    void testCarriageReturnClusters()
    {
        gran::text::FontLibrary fonts;
        const auto style = setupLayoutFont(fonts);
        const std::string shown = "a\r\n\r\n\rb";
        const std::vector<gran::text::TextRun> runs{{0, 2, style}, {2, 4, style}, {4, 6, style}, {6, 7, style}};
        const auto plain = gran::text::layoutText(shown, style, 10, fonts);
        const auto spanned = gran::text::layoutRuns(shown, runs, style, 10, fonts);
        const std::size_t offsets[]{0, 1, 1, 3, 3, 5, 5, 6, 7};
        for (const auto* layout : {&plain, &spanned})
        {
            TEST_ASSERT(layout->lines.size() == 4 && layout->carets.size() == 9);
            for (std::size_t index = 0; index < layout->carets.size(); ++index)
                TEST_ASSERT(gran::render::internal::caretStopOffset(*layout, index, shown) == offsets[index]);
            TEST_ASSERT(gran::render::internal::caretStopAt(*layout, 2, shown).cluster == 1);
        }
    }

    void testResourceFontCatalog()
    {
        static const auto font = namedReviewFont();
        static const std::uint8_t shortDirectory[]{0,1,0,0, 255,255, 0,0,0,0,0,0};
        const gran::res::Entry entries[]{{"font-catalog/named.ttf", font.data(), font.size()},
            {"font-catalog/short.ttf", shortDirectory, sizeof(shortDirectory)}};
        gran::res::mount({"font-catalog", entries, 2});
        std::vector<gran::text::FontCandidate> candidates;
        gran::text::scanFontFile("res:/font-catalog/named.ttf", candidates);
        gran::text::scanFontFile("res:/font-catalog/named.ttf", candidates);
        gran::text::scanFontFile("res:/font-catalog/short.ttf", candidates);
        gran::text::scanFontFile("res:/font-catalog/missing.ttf", candidates);
        TEST_ASSERT(candidates.size() == 1 && candidates.front().family == "Gran Review");

        gran::text::FontLibrary catalog;
        catalog.addFile("res:/font-catalog/named.ttf");
        catalog.addFile("res:/font-catalog/named.ttf");
        const auto nearest = catalog.match("gran review", 700, false);
        TEST_ASSERT(nearest && nearest->family() == "Gran Review" && nearest->metrics().weight == 400);
        TEST_ASSERT(catalog.match("GRAN REVIEW", 700, false) == nearest);
    }

    void testFailedFontRegistration()
    {
        static const auto healthy = namedReviewFont();
        static const auto damaged = [] {
            auto font = namedReviewFont();
            // Сканер читает name, однако для полной загрузки таблица head обязательна.
            // The scanner reads name, but a full load requires the head table.
            font[27] = 0;
            return font;
        }();
        const gran::res::Entry entries[]{{"font-catalog/retry.ttf", damaged.data(), damaged.size()},
            {"font-catalog/alternate.ttf", healthy.data(), healthy.size()}};
        gran::res::mount({"font-catalog-retry", entries, 2});
        gran::text::FontLibrary catalog;
        catalog.addFile("res:/font-catalog/retry.ttf");
        catalog.addFile("res:/font-catalog/alternate.ttf");
        const auto first = catalog.match("Gran Review", 400, false);
        TEST_ASSERT(first && first->family() == "Gran Review");

        const gran::res::Entry repaired{"font-catalog/retry.ttf", healthy.data(), healthy.size()};
        gran::res::mount({"font-catalog-repaired", &repaired, 1});
        catalog.addFile("res:/font-catalog/retry.ttf");
        const auto restored = catalog.match("Gran Review", 400, false);
        TEST_ASSERT(restored && restored != first && restored->family() == "Gran Review");
    }

    // Подтаблица PairPos описывает (1,1), а старый kern содержит также (1,2).
    // The PairPos subtable describes (1,1), and legacy kern also contains (1,2).
    void testGposKerningFallback()
    {
        const font_samples::Bytes kern{0,0, 0,1, 0,0, 0,26, 0,1, 0,2, 0,12, 0,1, 0,0,
            0,1, 0,1, 0,10, 0,1, 0,2, 0,20};
        font_samples::Bytes gpos{0,1, 0,0, 0,0, 0,10, 0,24,
            0,1, 'k','e','r','n', 0,8, 0,0, 0,1, 0,0,
            0,1, 0,4, 0,2, 0,0, 0,1, 0,8,
            0,1, 0,18, 0,4, 0,0, 0,1, 0,12, 0,1, 0,1, 0,30, 0,1, 0,1, 0,1};
        const auto glyph = font_samples::squareGlyph();
        const auto healthyPath = font_samples::writeTemp(font_samples::buildFont({glyph, glyph, glyph}, kern, gpos),
            "kerning_gpos_healthy.ttf");
        const auto healthy = gran::text::FontFace::load(gran::MappedFile::open(healthyPath), 0, healthyPath);
        TEST_ASSERT(healthy->kerning(1, 1) == 30);
        TEST_ASSERT(healthy->kerning(1, 2) == 0);

        // Увеличиваем PairValueCount без добавления записей.
        // Increase PairValueCount without adding records.
        gpos[49] = 8;
        const auto damagedPath = font_samples::writeTemp(font_samples::buildFont({glyph, glyph, glyph}, kern, gpos),
            "kerning_gpos_damaged.ttf");
        const auto damaged = gran::text::FontFace::load(gran::MappedFile::open(damagedPath), 0, damagedPath);
        TEST_ASSERT(damaged->kerning(1, 1) == 10);

        // Исправная пара сохраняет приоритет, сбой соседней подтаблицы разрешает fallback для остальных пар.
        // The healthy pair keeps priority, and damage to a sibling subtable allows fallback for other pairs.
        gpos[49] = 1;
        gpos[33] = 2;
        gpos[35] = 10;
        gpos.insert(gpos.begin() + 36, {0, 200});
        const auto mixedPath = font_samples::writeTemp(font_samples::buildFont({glyph, glyph, glyph}, kern, gpos),
            "kerning_gpos_mixed.ttf");
        const auto mixed = gran::text::FontFace::load(gran::MappedFile::open(mixedPath), 0, mixedPath);
        TEST_ASSERT(mixed->kerning(1, 1) == 30);
        TEST_ASSERT(mixed->kerning(1, 2) == 20);
    }

    // Матрица формата 2 не должна перекрывать таблицу покрытия.
    // The format 2 matrix must not overlap the coverage table.
    void testPairMatrixBounds()
    {
        std::uint8_t data[]{0,2, 0,18, 0,4, 0,0, 0,24, 0,30, 0,1, 0,1,
            0,30, 0,1, 0,1, 0,1, 0,1, 0,0, 0,0, 0,1, 0,0, 0,0};
        const gran::text::FontReader reader(data, sizeof(data));
        float value = 0;
        TEST_ASSERT(gran::text::pairAdjustment(reader, 0, 1, 1, value) == gran::text::PairResult::MATCH);
        TEST_ASSERT(value == 30);
        data[15] = 2;
        TEST_ASSERT(gran::text::pairAdjustment(reader, 0, 1, 1, value) == gran::text::PairResult::DAMAGED);
    }

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
        runner.add("text: run boundaries preserve wrapping and bounds", testTextRunBoundaries);
        runner.add("text: CR and split CRLF keep caret boundaries", testCarriageReturnClusters);
        runner.add("font: resource catalog deduplicates and rejects a short directory", testResourceFontCatalog);
        runner.add("font: catalog tries another face and can retry a re-registered font", testFailedFontRegistration);
        runner.add("font: prefers healthy GPOS and falls back from damaged pair data", testGposKerningFallback);
        runner.add("font: GPOS class matrix must end before its coverage", testPairMatrixBounds);
        runner.add("font: kern pair must fit its own table", [] {
            const std::uint8_t data[] = {0,0, 0,1, 0,0, 0,20, 0,1, 0,1, 0,0, 0,0, 0,0, 0,2, 0,3, 0,10};
            gran::text::FontReader reader(data, sizeof(data));
            TEST_ASSERT(gran::text::kernTableValue(reader, {0, sizeof(data)}, 2, 3) == 10);
            TEST_ASSERT(gran::text::kernTableValue(reader, {0, sizeof(data) - 2}, 2, 3) == 0);
        });
        runner.add("font: reads a name ending at the table boundary", [] {
            const std::uint8_t name[] = {0,0, 0,1, 0,18, 0,3, 0,1, 4,9, 0,1, 0,2, 0,0, 0,'A'};
            gran::text::FontReader reader(name, sizeof(name));
            TEST_ASSERT(gran::text::readFaceName(reader, {0, sizeof(name)}, false) == "A");
            TEST_ASSERT(gran::text::readFaceName(reader, {0, sizeof(name) - 1}, false).empty());
        });
        runner.add("font: preserves wide absolute glyph positions", [] {
            if (sizeof(std::size_t) > sizeof(std::uint32_t))
            {
                const std::uint8_t loca[] = {0, 2, 0, 8};
                gran::text::FontReader reader(loca, sizeof(loca));
                gran::text::FaceTables tables{};
                tables.glyf = {UINT32_MAX - 1u, 16};
                std::size_t begin = 0, end = 0;
                TEST_ASSERT(gran::text::glyphLocation(reader, tables, false, 0, begin, end));
                TEST_ASSERT(begin == UINT64_C(4294967298) && end == UINT64_C(4294967310));
            }
        });
        runner.add("font damage: composite whole", testFontDamageCompositeWhole);
        runner.add("font damage: composite truncated", testFontDamageCompositeTruncated);
        runner.add("font damage: component out of range", testFontDamageComponentOutOfRange);
        runner.add("font damage: simple truncated", testFontDamageSimpleTruncated);
        runner.add("font damage: composite loop", testFontDamageCompositeLoop);
    }
}
