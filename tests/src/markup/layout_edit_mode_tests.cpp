#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "support/layout_helpers.hpp"
#include "gran/app/application.hpp"
#include "gran/app/app_driver.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/markup/overlay.hpp"

namespace
{
    constexpr auto MARKUP = R"(<Window><Resources>
        <Command id="replacement" text="Replacement" action="replaceAction"/>
        <Command id="privateCommand" text="Private" action="privateAction"/>
        <Palette><CommandRef ref="replacement"/></Palette>
        </Resources><Stack>
        <Button id="b" text="Button" customizable="command" onClick="legacy" popup="pop"/>
        <Button id="locked" text="Locked"/>
        <Box id="box" customizable="command"/>
        <Popup id="pop"><Text>Popup</Text></Popup>
        </Stack></Window>)";

    void testRebindCommand()
    {
        using namespace gran::markup;
        const auto base = loadMarkupString(MARKUP, {});
        const auto result = applyOverlay(base,
            R"(<Overlay version="1"><Rebind id="b" command="replacement"/></Overlay>)", {});
        const auto& button = support::nodeById(result.document, "b");
        TEST_ASSERT(button.content.command == "replacement");
        TEST_ASSERT(button.content.onClick.empty());
        TEST_ASSERT(button.content.popup.empty());
        TEST_ASSERT(support::nodeById(base, "b").content.onClick == "legacy");
        TEST_ASSERT(support::nodeById(base, "b").content.popup == "pop");
    }

    void testForbiddenRebind()
    {
        using namespace gran::markup;
        const auto base = loadMarkupString(MARKUP, {});
        TEST_ASSERT_THROWS(applyOverlay(base, R"(<Overlay version="1">
            <Rebind id="b" command="privateCommand"/>
            </Overlay>)", {}), gran::GranException);
        TEST_ASSERT_THROWS(applyOverlay(base, R"(<Overlay version="1">
            <Rebind id="locked" command="replacement"/>
            </Overlay>)", {}), gran::GranException);
        TEST_ASSERT_THROWS(applyOverlay(base, R"(<Overlay version="1">
            <Rebind id="box" command="replacement"/>
            </Overlay>)", {}), gran::GranException);
    }

    void testHeadlessApplication()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window><Resources>
            <Command id="customize" action="gran:customize" key="Mod+Shift+F12"/>
            <Command id="profiles" action="gran:profiles" key="Mod+F12"/>
            </Resources><Box/></Window>)");
        gran::AppDriver driver(app);
        driver.start();
        using namespace gran::platform;
        driver.press(Key::F12, MODIFIER_CONTROL | MODIFIER_SHIFT);
        TEST_ASSERT(app.editMode() && driver.windowCount() == 1);
        driver.press(Key::F12, MODIFIER_CONTROL);
        TEST_ASSERT(driver.windowCount() == 2);
        TEST_ASSERT(app.editMode());
    }

    void testInlineInspector()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        app.setEditMode(true);
        driver.click(*app.find("b"), gran::platform::MouseButton::RIGHT);
        auto* dialog = gran::ui::popupOf(app.find("destination"));
        TEST_ASSERT(dialog && dialog->kind == gran::ui::ElementKind::DIALOG && dialog->visible);
        TEST_ASSERT(driver.windowCount() == 1);
        const auto action = [](gran::ui::Node& parent, const char* name, const auto& find) -> gran::ui::Node* {
            if (parent.content.onClick == name)
                return &parent;
            for (auto& child : parent.children)
                if (auto* node = find(*child, name, find)) return node;
            return nullptr;
        };
        auto* close = action(*dialog, "gran:layout:dismiss", action);
        TEST_ASSERT(close);
        driver.click(*close);
        TEST_ASSERT(!dialog->visible && app.editMode());
        driver.click(*app.find("b"), gran::platform::MouseButton::RIGHT);
        auto* source = app.find("source");
        dialog = gran::ui::popupOf(source);
        TEST_ASSERT(dialog && dialog->visible && source);
        auto* tab = source->parent;
        while (tab && tab->kind != gran::ui::ElementKind::TAB)
            tab = tab->parent;
        TEST_ASSERT(tab && gran::ui::headerOfPage(*tab));
        driver.click(*gran::ui::headerOfPage(*tab));
        app.selectOption(*source, "c:replacement");
        driver.settle();
        auto* apply = action(*dialog, "gran:layout:rebind", action);
        TEST_ASSERT(apply && apply->layout.frame.width > 0);
        driver.click(*apply);
        TEST_ASSERT(app.find("b")->content.command == "replacement");
    }
}

namespace suites
{
    void registerLayoutEditModeTests(test::TestRunner& runner)
    {
        runner.add("layout: replaces a button command", testRebindCommand);
        runner.add("layout: rejects unauthorized command replacement", testForbiddenRebind);
        runner.add("layout: inline editing and separate profile shortcut", testHeadlessApplication);
        runner.add("layout: inspector opens in place and applies a command", testInlineInspector);
    }
}
