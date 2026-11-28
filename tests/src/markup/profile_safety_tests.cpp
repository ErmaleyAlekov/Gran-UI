#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/markup/overlay.hpp"

namespace
{
    void testRuntimeTabSelection()
    {
        gran::Application app;
        app.loadString(R"(<Window><Resources><Theme><Token name="size" type="size" value="12"/></Theme>
            </Resources><Tabs id="tabs"><Tab id="initial" text="Initial"/></Tabs></Window>)");
        auto first = std::make_unique<gran::ui::Node>();
        first->id = "runtimeFirst";
        first->kind = gran::ui::ElementKind::TAB;
        first->content.option = first->id;
        auto second = std::make_unique<gran::ui::Node>();
        second->id = "runtimeSecond";
        second->kind = gran::ui::ElementKind::TAB;
        second->content.option = second->id;
        auto* keptFirst = &app.append(*app.find("tabs"), std::move(first));
        auto* keptSecond = &app.append(*app.find("tabs"), std::move(second));
        gran::ui::syncTabs(*app.find("tabs"));
        app.selectTab(*app.find("tabs"), "runtimeSecond");
        TEST_ASSERT(keptSecond->visible && !keptFirst->visible);
        app.importProfile("scale", R"(<Overlay version="1"><Token name="size" value="14"/></Overlay>)");
        TEST_ASSERT(app.find("runtimeFirst") == keptFirst && app.find("runtimeSecond") == keptSecond);
        TEST_ASSERT(keptSecond->visible && !keptFirst->visible);
        TEST_ASSERT(app.find("tabs")->content.option == "runtimeSecond");
    }

    void testRuntimeIdCollision()
    {
        gran::Application app;
        app.loadString(R"(<Window><Resources><Command id="save" text="Save"/>
            <Palette><CommandRef ref="save"/></Palette></Resources>
            <Stack id="host" customizable="insert"/></Window>)");
        auto item = std::make_unique<gran::ui::Node>();
        item->kind = gran::ui::ElementKind::TEXT;
        item->id = "occupied";
        auto* original = &app.append(*app.find("host"), std::move(item));
        TEST_ASSERT_THROWS(app.importProfile("collision",
            R"(<Overlay version="1"><Insert id="occupied" parent="host" command="save"/></Overlay>)"),
            gran::GranException);
        TEST_ASSERT(app.find("occupied") == original && app.profiles().empty());
    }

    void testContrastRollback()
    {
        using namespace gran::markup;
        const auto base = loadMarkupString(R"(<Window background="#ffffff" minTextContrast="4.5">
            <Resources><Theme><Token name="ink" type="color" value="#000000"/></Theme></Resources>
            <Text color="$ink">Readable</Text></Window>)", {});
        const auto good = applyOverlay(base, R"(<Overlay version="1"><Token name="ink" value="#222222"/></Overlay>)", {});
        TEST_ASSERT(good.document.tokens.at("ink") == "#222222");
        TEST_ASSERT_THROWS(applyOverlay(base,
            R"(<Overlay version="1"><Token name="ink" value="#cccccc"/></Overlay>)", {}), gran::MarkupException);
        TEST_ASSERT(base.tokens.at("ink") == "#000000");
    }

    void testGradientContrast()
    {
        using namespace gran::markup;
        const auto loadGradient = [](const char* foreground, const char* ratio)
        {
            const auto xml = std::string("<Window minTextContrast=\"") + ratio + "\"><Text color=\""
                + foreground + R"xml(" background="linear-gradient(90deg, #ff0000, #0000ff)">Text</Text></Window>)xml";
            return loadMarkupString(xml, {});
        };
        const auto passing = loadGradient("#ffffff", "3.5");
        TEST_ASSERT(passing.root->content.text == "Text");
        TEST_ASSERT_THROWS(loadGradient("#ffffff", "4.5"), gran::MarkupException);
        TEST_ASSERT_THROWS(loadGradient("#000000", "2.3"), gran::MarkupException);
        TEST_ASSERT_THROWS(loadMarkupString(R"xml(<Window background="#ffffff" minTextContrast="6.8488126">
            <Text color="#000000" background="linear-gradient(90deg, #49a6c9, #cc6ffd)">Text</Text>
            </Window>)xml", {}), gran::MarkupException);
        const auto transparent = loadMarkupString(R"xml(<Window minTextContrast="3.5">
            <Stack background="linear-gradient(90deg, #ff0000, #0000ff)">
                <Text color="#ffffff" background="linear-gradient(90deg, rgba(255,255,255,0), rgba(0,0,0,0))">Text</Text>
            </Stack></Window>)xml", {});
        TEST_ASSERT(transparent.root->children.front()->content.text == "Text");
    }

    void testSafeStartup()
    {
        gran::AppOptions normal;
        gran::AppOptions safe;
        normal.headless = safe.headless = true;
        safe.skipCustomization = true;
        gran::Application activated(normal);
        gran::Application bypassed(safe);
        const auto markup = R"(<Window><Stack customizable="hide"><Box id="item"/></Stack></Window>)";
        const auto overlay = R"(<Overlay version="1"><Hide id="item"/></Overlay>)";
        for (auto* app : {&activated, &bypassed})
        {
            app->settings().setString("gran.activeProfile", "home");
            app->settings().setString("gran.profile.home", overlay);
            app->loadString(markup);
        }
        TEST_ASSERT(!activated.find("item")->visible);
        TEST_ASSERT(bypassed.find("item")->visible);
        TEST_ASSERT(bypassed.settings().getString("gran.activeProfile") == "home");
        TEST_ASSERT(bypassed.exportProfile("home") == overlay);
    }

    void testTransformedTargetSize()
    {
        using namespace gran::markup;
        TEST_ASSERT_THROWS(loadMarkupString(R"xml(<Window minTargetSize="44">
            <Button text="Tap" width="44" height="44"><Pressed transform="scale(0.5)"/></Button>
            </Window>)xml", {}), gran::MarkupException);
        TEST_ASSERT_THROWS(loadMarkupString(R"xml(<Window minTargetSize="44">
            <Stack><Pressed transform="scale(2)"/>
                <Button text="Tap" width="44" height="44"><Pressed transform="scale(0.5)"/></Button>
            </Stack></Window>)xml", {}), gran::MarkupException);
        TEST_ASSERT_THROWS(loadMarkupString(R"xml(<Window minTargetSize="44">
            <Button text="Tap" width="88" height="88" transform="scale(0)"/>
            </Window>)xml", {}), gran::MarkupException);
        const auto accepted = loadMarkupString(R"xml(<Window minTargetSize="44"><Stack>
            <Button text="Large" width="88" height="88"><Pressed transform="scale(0.5)"/></Button>
            <Button text="Rotated" width="44" height="44" transform="rotate(90deg)">
                <Pressed transform="rotate(90deg)"/>
            </Button>
            <Button text="Automatic"><Pressed transform="scale(0.5)"/></Button>
            </Stack></Window>)xml", {});
        const auto& automatic = *accepted.root->children.at(2);
        gran::style::CascadeInput input;
        input.inlineBlock = &automatic.declared;
        const auto style = gran::style::computeStyle(input);
        TEST_ASSERT(style.minWidth == 88 && style.minHeight == 88);
        const auto rotated = loadMarkupString(R"xml(<Window minTargetSize="44">
            <Stack transform="scale(0.5, 1)"><Hover transform="scale(1, 0.5)"/>
                <Button text="Rotated" width="70" height="70" transform="rotate(45deg)">
                    <Pressed transform="rotate(45deg)"/>
                </Button>
            </Stack></Window>)xml", {});
        TEST_ASSERT(rotated.root->children.front()->content.text == "Rotated");
    }

    void testDeepTransformStates()
    {
        using namespace gran::markup;
        std::string markup = R"(<Window minTargetSize="44">)";
        for (int hundredths = 90; hundredths <= 96; ++hundredths)
        {
            const auto scale = "0." + std::to_string(hundredths);
            markup += "<Stack transform=\"scale(" + scale + ",1)\"><Hover transform=\"scale(1," + scale + ")\"/>";
        }
        markup += R"xml(<Button id="target" text="Rotated" width="64" height="64" transform="rotate(45deg)">
            <Pressed transform="rotate(45deg)"/></Button>)xml";
        for (int level = 0; level < 7; ++level)
            markup += "</Stack>";
        markup += "</Window>";
        const auto document = loadMarkupString(markup, {});
        TEST_ASSERT(gran::ui::findNode(*document.root, "target") != nullptr);
    }
}

namespace suites
{
    void registerProfileSafetyTests(test::TestRunner& runner)
    {
        runner.add("customization: selected runtime Tab survives profile", testRuntimeTabSelection);
        runner.add("customization: runtime id collision rolls back", testRuntimeIdCollision);
        runner.add("customization: text contrast violation rolls back", testContrastRollback);
        runner.add("customization: gradient contrast uses actual color bounds", testGradientContrast);
        runner.add("customization: safe startup skips the stored profile", testSafeStartup);
        runner.add("customization: minimum target size follows transforms", testTransformedTargetSize);
        runner.add("customization: target size checks reachable deep transform states", testDeepTransformStates);
    }
}
