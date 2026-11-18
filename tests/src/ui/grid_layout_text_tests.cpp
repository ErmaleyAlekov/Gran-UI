#include "gran/ui/data_grid.hpp"
#include "gran/style/value_parsers.hpp"
#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"

#include <limits>

namespace
{
    std::vector<gran::ui::GridColumnSpec> makeSpecs()
    {
        std::vector<gran::ui::GridColumnSpec> specs(3);
        specs[0].column.key = "a";
        specs[1].column.key = "b";
        specs[2].column.key = "c";
        return specs;
    }

    void testGridLayoutText()
    {
        gran::ui::GridState state;
        state.columns = {
            {"a", "Title A", "a", gran::style::parseTrack("auto"), 0.0f, std::numeric_limits<float>::infinity()},
            {"b", "Title B", "b", gran::style::parseTrack("120"), 0.0f, std::numeric_limits<float>::infinity(), true},
            {"c", "Title C", "c", gran::style::parseTrack("auto"), 0.0f, std::numeric_limits<float>::infinity()},
            {"", "Title D", "", gran::style::parseTrack("auto"), 0.0f, std::numeric_limits<float>::infinity(), true}
        };

        // Седьмое поле агрегата — sortable, поэтому resized ставится по имени / The seventh aggregate field is sortable, so resized is set by name
        state.columns[1].resized = true;
        state.columns[3].resized = true;
        TEST_ASSERT(gran::ui::gridLayoutText(state) == "a b:120 c");

        state.columns[1].width = gran::style::parseTrack("120.25");
        TEST_ASSERT(gran::ui::gridLayoutText(state) == "a b:120.3 c");

        state.columns[2].width = gran::style::parseTrack("auto");
        state.columns[2].resized = true;
        TEST_ASSERT(gran::ui::gridLayoutText(state) == "a b:120.3 c:auto");
    }

    void testApplyGridLayout()
    {
        auto specs = makeSpecs();

        TEST_ASSERT(gran::ui::applyGridLayout(specs, "c a:80"));
        TEST_ASSERT(specs[0].column.key == "c");
        TEST_ASSERT(specs[1].column.key == "a");
        TEST_ASSERT(specs[2].column.key == "b");
        TEST_ASSERT(specs[1].column.resized);
        TEST_ASSERT(specs[1].column.width.min.kind == gran::style::TrackBoundKind::PIXELS);
        TEST_ASSERT(specs[1].column.width.max.kind == gran::style::TrackBoundKind::PIXELS);
        TEST_ASSERT_NEAR(specs[1].column.width.min.value, 80.0f, 0.001f);
        TEST_ASSERT_NEAR(specs[1].column.width.max.value, 80.0f, 0.001f);
        TEST_ASSERT(!specs[0].column.resized);
        TEST_ASSERT(!specs[2].column.resized);

        specs = makeSpecs();
        TEST_ASSERT(gran::ui::applyGridLayout(specs, "zz b"));
        TEST_ASSERT(specs[0].column.key == "b");
        TEST_ASSERT(specs[1].column.key == "a");
        TEST_ASSERT(specs[2].column.key == "c");

        TEST_ASSERT(!gran::ui::applyGridLayout(specs, ""));
        TEST_ASSERT(specs[0].column.key == "b");
        TEST_ASSERT(specs[1].column.key == "a");
        TEST_ASSERT(specs[2].column.key == "c");

        specs = makeSpecs();
        TEST_ASSERT(gran::ui::applyGridLayout(specs, "a:abc b:-5 c:1e999"));
        TEST_ASSERT(specs[0].column.key == "a");
        TEST_ASSERT(specs[1].column.key == "b");
        TEST_ASSERT(specs[2].column.key == "c");
        TEST_ASSERT(!specs[0].column.resized);
        TEST_ASSERT(!specs[1].column.resized);
        TEST_ASSERT(!specs[2].column.resized);

        specs = makeSpecs();
        TEST_ASSERT(gran::ui::applyGridLayout(specs, "b b a"));
        TEST_ASSERT(specs[0].column.key == "b");
        TEST_ASSERT(specs[1].column.key == "a");
        TEST_ASSERT(specs[2].column.key == "c");
        TEST_ASSERT(specs.size() == 3);

        specs = makeSpecs();
        TEST_ASSERT(gran::ui::applyGridLayout(specs, "a:auto"));
        TEST_ASSERT(specs[0].column.key == "a");
        TEST_ASSERT(specs[0].column.resized);
        TEST_ASSERT(specs[0].column.width.max.kind == gran::style::TrackBoundKind::AUTO);

        gran::ui::GridState state;
        state.columns = {
            {"c", "Title C", "c", gran::style::parseTrack("120"), 0.0f, std::numeric_limits<float>::infinity()},
            {"a", "Title A", "a", gran::style::parseTrack("80"), 0.0f, std::numeric_limits<float>::infinity()},
            {"b", "Title B", "b", gran::style::parseTrack("auto"), 0.0f, std::numeric_limits<float>::infinity()}
        };
        for (gran::ui::GridColumn& column : state.columns)
            column.resized = true;
        state.widths = {80.0f, 120.0f, 120.0f};

        specs = makeSpecs();
        TEST_ASSERT(gran::ui::applyGridLayout(specs, gran::ui::gridLayoutText(state)));
        TEST_ASSERT(specs[0].column.key == "c");
        TEST_ASSERT(specs[1].column.key == "a");
        TEST_ASSERT(specs[2].column.key == "b");
        TEST_ASSERT(specs[0].column.resized && specs[1].column.resized && specs[2].column.resized);
        TEST_ASSERT(specs[0].column.width.max.kind == gran::style::TrackBoundKind::PIXELS);
        TEST_ASSERT_NEAR(specs[0].column.width.max.value, 120.0f, 0.001f);
        TEST_ASSERT(specs[1].column.width.min.kind == gran::style::TrackBoundKind::PIXELS);
        TEST_ASSERT_NEAR(specs[1].column.width.min.value, 80.0f, 0.001f);
        TEST_ASSERT(specs[2].column.width.min.kind == gran::style::TrackBoundKind::AUTO);
        TEST_ASSERT(specs[2].column.width.max.kind == gran::style::TrackBoundKind::AUTO);
    }
}

namespace suites
{
    void registerGridLayoutTextTests(test::TestRunner& runner)
    {
        runner.add("grid layout text: gridLayoutText", testGridLayoutText);
        runner.add("grid layout text: applyGridLayout", testApplyGridLayout);
    }
}
