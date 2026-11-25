#include "gran/core/gran_exception.hpp"
#include <limits>
#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/ui/node.hpp"
#include "gran/render/text_engine.hpp"
#include "gran/render/internal/text_run_bounds.hpp"
#include <string>

namespace
{
    void testDisplayedTextPassword()
    {
        gran::ui::Node node;
        node.kind = gran::ui::ElementKind::INPUT;
        node.content.text = u8"аб1";
        node.content.password = true;

        std::string displayed = gran::render::displayedText(node);
        TEST_ASSERT(displayed == std::string(gran::render::PASSWORD_MASK) + std::string(gran::render::PASSWORD_MASK) + std::string(gran::render::PASSWORD_MASK));
        node.kind = gran::ui::ElementKind::TEXT_AREA;
        TEST_ASSERT(gran::render::displayedText(node) == node.content.text);
        TEST_ASSERT(gran::render::toDisplayOffset(node, 2) == 2);
    }

    void testToDisplayOffsetPassword()
    {
        gran::ui::Node node;
        node.kind = gran::ui::ElementKind::INPUT;
        node.content.text = u8"аб1";
        node.content.password = true;

        TEST_ASSERT(gran::render::toDisplayOffset(node, 2) == 3);
        TEST_ASSERT(gran::render::toDisplayOffset(node, 5) == 9);
    }

    void testToContentOffsetPassword()
    {
        gran::ui::Node node;
        node.kind = gran::ui::ElementKind::INPUT;
        node.content.text = u8"аб1";
        node.content.password = true;

        TEST_ASSERT(gran::render::toContentOffset(node, 3) == 2);
        TEST_ASSERT(gran::render::toContentOffset(node, 6) == 4);
        TEST_ASSERT(gran::render::toContentOffset(node, 9) == 5);
    }

    void testDisplayedTextNoPassword()
    {
        gran::ui::Node node;
        node.kind = gran::ui::ElementKind::INPUT;
        node.content.text = u8"аб1";

        std::string displayed = gran::render::displayedText(node);
        TEST_ASSERT(displayed == u8"аб1");

        TEST_ASSERT(gran::render::toDisplayOffset(node, 4) == 4);
        TEST_ASSERT(gran::render::toContentOffset(node, 4) == 4);
    }

    void testDisplayedTextPlaceholder()
    {
        gran::ui::Node node;
        node.kind = gran::ui::ElementKind::INPUT;
        node.content.placeholder = u8"Пароль";
        node.content.password = true;

        std::string displayed = gran::render::displayedText(node);
        TEST_ASSERT(displayed == u8"Пароль");
    }
}

namespace suites
{
    void registerPasswordMaskTests(test::TestRunner& runner)
    {
        runner.add("render: finite width has a distinct layout key", [] {
            gran::text::TextStyle style;
            TEST_ASSERT(gran::render::layoutKey("a", style, -1.0f / 64) !=
                gran::render::layoutKey("a", style, std::numeric_limits<float>::infinity()));
        });
        runner.add("render: checks span byte offsets before addition", [] {
            using gran::render::internal::textRunEnd;
            TEST_ASSERT(textRunEnd(UINT32_MAX - 1, 1) == UINT32_MAX);
            bool rejected = false;
            try { textRunEnd(UINT32_MAX - 1, 2); }
            catch (const gran::GranException& error) { rejected = error.getCode() == gran::ErrorCode::LIMIT_EXCEEDED; }
            TEST_ASSERT(rejected);
        });
        runner.add("render: displayedText password", testDisplayedTextPassword);
        runner.add("render: toDisplayOffset password", testToDisplayOffsetPassword);
        runner.add("render: toContentOffset password", testToContentOffsetPassword);
        runner.add("render: displayedText no password", testDisplayedTextNoPassword);
        runner.add("render: displayedText placeholder", testDisplayedTextPlaceholder);
    }
}
