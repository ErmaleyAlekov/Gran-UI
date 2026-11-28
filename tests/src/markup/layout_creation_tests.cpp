#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "support/layout_helpers.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/markup/overlay.hpp"
#include "beresta.hpp"

namespace layout_tests { void testCreateLiveButton(); void testCreateAndMoveTabs(); void testLiveInputMovePolicy(); }

namespace
{
    gran::markup::UiDocument palette()
    {
        return gran::markup::loadMarkupString(R"(<Window><Resources>
          <Theme><Token name="size" value="12" type="size" min="8" max="20"/></Theme>
          <Command id="save" text="Save"/><Command id="open" text="Open"/>
          <Palette><CommandRef ref="save"/><CommandRef ref="open"/>
            <Template name="tool"><Button text="Tool" customizable="move style command" onClick="original"><Text id="command">Tool</Text></Button></Template>
            <Template name="field"><Input customizable="style" fontSize="$size" accessibleName="Field"/></Template>
            <Template name="group"><Stack><Text id="label">Notes</Text>
                <Input id="entry" labeledBy="label" customizable="move"/></Stack></Template>
            <Template name="page"><Tab customizable="move style" text="User page" value="prototype"/></Template>
          </Palette></Resources>
          <Stack><Stack id="tools" customizable="insert"/><Stack id="more" customizable="insert"/>
            <Stack id="closed" customizable=""/><Tabs id="tabs" customizable="insert">
              <Tab id="existing" text="Existing" value="existing"/>
            </Tabs></Stack></Window>)", {});
    }

    void testPaletteButton()
    {
        using namespace gran::markup;
        auto base = palette();
        auto result = applyOverlay(base, R"(<Overlay version="1">
            <Insert id="button" parent="tools" template="tool" sequence="save open" text="Save and open"/>
            <Insert id="cleared" parent="tools" template="tool" text=""/>
            <Move id="button" parent="more"/><Attribute id="button" name="width" value="90"/>
            </Overlay>)", {});
        const auto& button = support::nodeById(result.document, "button");
        TEST_ASSERT(support::nodeById(result.document, "cleared").domElement->getAttribute("text") == "");
        TEST_ASSERT(button.kind == gran::ui::ElementKind::BUTTON);
        TEST_ASSERT(button.parent->id == "more");
        TEST_ASSERT(button.domElement->getAttribute("command") == "button.command1");
        TEST_ASSERT(button.domElement->getAttribute("text") == "Save and open");
        TEST_ASSERT(button.domElement->getAttribute("width") == "90");
        TEST_ASSERT(!button.domElement->hasAttribute("onClick"));
        TEST_ASSERT((result.document.commands.back().sequence == std::vector<std::string>{"save", "open"}));
    }

    void testPaletteField()
    {
        using namespace gran::markup;
        auto base = palette();
        auto result = applyOverlay(base, R"(<Overlay version="1">
            <Insert id="field" parent="tools" template="field" token="size" value="18"/>
            <Insert id="peer" parent="tools" template="field"/>
            <Insert id="one" parent="tools" template="group"/><Insert id="two" parent="tools" template="group"/>
            <Move id="two.entry" parent="more"/>
            </Overlay>)", {});
        const auto& field = support::nodeById(result.document, "field");
        TEST_ASSERT(field.domElement->getLocalName() == "Input");
        TEST_ASSERT(field.domElement->getAttribute("fontSize") == "18");
        TEST_ASSERT(support::nodeById(result.document, "peer").domElement->getAttribute("fontSize") == "$size");
        TEST_ASSERT(result.document.tokens.at("size") == "12");
        TEST_ASSERT(support::nodeById(result.document, "two.entry").parent->id == "more");
        TEST_ASSERT(support::nodeById(result.document, "one.entry").domElement->getAttribute("labeledBy") == "one.label");
    }

    void testPaletteTabs()
    {
        using namespace gran::markup;
        auto base = palette();
        auto result = applyOverlay(base, R"(<Overlay version="1">
            <Insert id="first" parent="tabs" template="page"/>
            <Insert id="second" parent="tabs" template="page" before="first"/>
            </Overlay>)", {});
        const auto& first = support::nodeById(result.document, "first");
        const auto& second = support::nodeById(result.document, "second");
        TEST_ASSERT(first.kind == gran::ui::ElementKind::TAB && second.kind == gran::ui::ElementKind::TAB);
        TEST_ASSERT(first.domElement->getAttribute("value") == "first");
        TEST_ASSERT(second.domElement->getAttribute("value") == "second");
        TEST_ASSERT(second.domElement->getIndexInParent() < first.domElement->getIndexInParent());
    }

    void testLongScopedStyleChain()
    {
        using namespace gran::markup;
        constexpr int styleCount = 256;
        std::string markup = R"(<Window><Resources><Theme><Token name="size" type="size" value="12"/>
            </Theme><Style name="s0" fontSize="$size" letterSpacing="1"/>)";
        for (int index = 1; index < styleCount; ++index)
            markup += "<Style name=\"s" + std::to_string(index) + "\" basedOn=\"s"
                + std::to_string(index - 1) + "\" letterSpacing=\"2\"/>";
        markup += R"(</Resources><Stack><Button id="local" text="Local" style="s255" customizable="style"/>
            <Button id="peer" text="Peer" style="s255"/></Stack></Window>)";
        const auto base = loadMarkupString(markup, {});
        const auto result = applyOverlay(base, R"(<Overlay version="1"><Token id="local" name="size" value="18"/>
            </Overlay>)", {});
        const auto* local = support::nodeById(result.document, "local").rule;
        const auto* peer = support::nodeById(result.document, "peer").rule;
        gran::style::CascadeInput input;
        input.rule = local;
        const auto localStyle = gran::style::computeStyle(input);
        input.rule = peer;
        const auto peerStyle = gran::style::computeStyle(input);
        for (int index = styleCount - 1; index >= 0; --index)
        {
            TEST_ASSERT(local && peer && local != peer && peer->name == "s" + std::to_string(index));
            local = local->basedOn;
            peer = peer->basedOn;
        }
        TEST_ASSERT(!local && !peer);
        TEST_ASSERT(localStyle.fontSize == 18 && localStyle.letterSpacing == 2);
        TEST_ASSERT(peerStyle.fontSize == 12 && peerStyle.letterSpacing == 2);
    }

    void testPaletteRejectsInvalidOperations()
    {
        using namespace gran::markup;
        auto base = palette();
        TEST_ASSERT_THROWS(applyOverlay(base, R"(<Overlay version="1">
            <Insert id="tool" parent="closed" template="tool"/></Overlay>)", {}), gran::MarkupException);
        TEST_ASSERT_THROWS(applyOverlay(base, R"(<Overlay version="1">
            <Insert id="page" parent="tools" template="page"/></Overlay>)", {}), gran::MarkupException);
        TEST_ASSERT_THROWS(applyOverlay(base, R"(<Overlay version="1">
            <Insert id="field" parent="tools" template="field"/>
            <Move id="field" parent="more"/></Overlay>)", {}), gran::MarkupException);
        TEST_ASSERT_THROWS(applyOverlay(base, R"(<Overlay version="1">
            <Insert id="field" parent="tools" template="field"/>
            <Rebind id="field" command="save"/></Overlay>)", {}), gran::MarkupException);
        TEST_ASSERT_THROWS(applyOverlay(base, R"(<Overlay version="1">
            <Insert id="field" parent="tools" template="field" token="size" value="21"/>
            </Overlay>)", {}), gran::MarkupException);
        TEST_ASSERT(support::nodeById(base, "tools").children.empty());
    }
}

namespace suites
{
    void registerLayoutCreationTests(test::TestRunner& runner)
    {
        runner.add("layout creation: moving live fields respects individual permissions", layout_tests::testLiveInputMovePolicy);
        runner.add("layout creation: create and reorder live tab pages", layout_tests::testCreateAndMoveTabs);
        runner.add("layout creation: shortcut creates a working paste and save button", layout_tests::testCreateLiveButton);
        runner.add("layout creation: configure and move palette button", testPaletteButton);
        runner.add("layout creation: apply scoped style to palette input", testPaletteField);
        runner.add("layout creation: create independent palette tabs", testPaletteTabs);
        runner.add("layout creation: scoped token copies a long style chain", testLongScopedStyleChain);
        runner.add("layout creation: reject invalid palette operations", testPaletteRejectsInvalidOperations);
    }
}
