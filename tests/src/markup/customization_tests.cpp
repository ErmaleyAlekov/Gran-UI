#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "support/layout_helpers.hpp"
#include "gran/app/application.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/markup/overlay.hpp"

namespace
{
    void testCustomizationHide()
    {
        using namespace gran::markup;
        auto base = loadMarkupString(R"(<Window><Stack customizable="hide">
            <Box id="open"/><Stack customizable="move"><Box id="closed"/></Stack>
            </Stack></Window>)", {});
        auto result = applyOverlay(base,
            R"(<Overlay version="1"><Hide id="open"/><Hide id="missing"/></Overlay>)", {});
        TEST_ASSERT(!support::nodeById(result.document, "open").visible);
        TEST_ASSERT(support::nodeById(base, "open").visible);
        TEST_ASSERT(result.missingIds == std::vector<std::string>{"missing"});
        TEST_ASSERT_THROWS(applyOverlay(base,
            R"(<Overlay version="1"><Hide id="open"/><Hide id="closed"/></Overlay>)", {}), gran::MarkupException);
        TEST_ASSERT(support::nodeById(base, "open").visible);
        TEST_ASSERT(support::nodeById(base, "closed").visible);
    }

    void testMoveAndInsert()
    {
        using namespace gran::markup;
        auto base = loadMarkupString(R"(<Window><Resources>
            <Command id="save" text="Save" action="save"/>
            <Palette><CommandRef ref="save"/></Palette>
            </Resources><Stack><Stack id="left"><Box id="item" customizable="move"/></Stack>
            <Stack id="right" customizable="insert"/></Stack></Window>)", {});
        auto result = applyOverlay(base, R"(<Overlay version="1">
            <Move id="item" parent="right"/><Insert id="button" parent="right" command="save" before="item"/>
            </Overlay>)", {});
        TEST_ASSERT(result.missingIds.empty());
        const auto& right = support::nodeById(result.document, "right");
        TEST_ASSERT(right.children.size() == 2);
        TEST_ASSERT(right.children[0]->id == "button");
        TEST_ASSERT(right.children[0]->kind == gran::ui::ElementKind::BUTTON);
        TEST_ASSERT(right.children[1]->id == "item");
        TEST_ASSERT(support::nodeById(base, "left").children.size() == 1);
        TEST_ASSERT_THROWS(loadMarkupString(R"(<Window><Resources>
            <Palette><CommandRef ref="unknown"/></Palette></Resources><Box/></Window>)", {}), gran::MarkupException);
    }

    void testTokenNumericSizeRange()
    {
        using namespace gran::markup;
        auto base = loadMarkupString(R"(<Window><Resources><Theme>
            <Token name="size" value="10" type="size" min="8" max="20"/>
            </Theme></Resources><Box width="$size"/></Window>)", {});
        auto result = applyOverlay(base, R"(<Overlay version="1"><Token name="size" value="20"/></Overlay>)", {});
        TEST_ASSERT(result.document.tokens.at("size") == "20");
        TEST_ASSERT(base.tokens.at("size") == "10");
        TEST_ASSERT_THROWS(applyOverlay(base,
            R"(<Overlay version="1"><Token name="size" value="21"/></Overlay>)", {}), gran::MarkupException);
    }

    void testProfiles()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window><Stack customizable="hide"><Box id="item"/></Stack></Window>)");
        const std::string hidden = R"(<Overlay version="1"><Hide id="item"/></Overlay>)";
        const std::string normal = R"(<Overlay version="1"/>)";
        app.importProfile("hidden", hidden);
        TEST_ASSERT(!app.find("item")->visible);
        app.importProfile("normal", normal);
        TEST_ASSERT(app.find("item")->visible);
        TEST_ASSERT((app.profiles() == std::vector<std::string>{"hidden", "normal"}));
        TEST_ASSERT(app.exportProfile("hidden") == hidden);
        app.selectProfile("hidden");
        TEST_ASSERT(!app.find("item")->visible);
        TEST_ASSERT_THROWS(app.importProfile("invalid",
            R"(<Overlay version="1"><Attribute id="item" name="width" value="50"/></Overlay>)"), gran::MarkupException);
        TEST_ASSERT(!app.find("item")->visible);
        TEST_ASSERT((app.profiles() == std::vector<std::string>{"hidden", "normal"}));
    }
}

namespace suites
{
    void registerCustomizationTests(test::TestRunner& runner)
    {
        runner.add("customization: nearest permission and rollback", testCustomizationHide);
        runner.add("customization: move and palette insertion", testMoveAndInsert);
        runner.add("customization: editable token range", testTokenNumericSizeRange);
        runner.add("customization: profile switch and invalid import", testProfiles);
    }
}
