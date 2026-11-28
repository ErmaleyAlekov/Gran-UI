#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/app/app_driver.hpp"

namespace layout_tests
{
    void testPointerMovesButton();
    void testMiddleClickDoesNotCloseTabWhileEditing();
    void testPointerReordersVerticalTabBar();
}

namespace
{
    gran::ui::Node* moveButton(gran::ui::Node& node)
    {
        if (node.content.onClick == "gran:layout:move") return &node;
        for (auto& child : node.children)
            if (auto* button = moveButton(*child)) return button;
        return nullptr;
    }

    void testInspectorMoveDestinations()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window><Stack orientation="horizontal">
            <Stack id="parent" customizable="insert"><Stack id="target" width="100" height="80" customizable="move insert">
                <Stack id="descendant" width="10" height="10" customizable="insert"/>
            </Stack></Stack><Stack id="other" customizable="insert" width="100" height="80"/>
            </Stack></Window>)");
        gran::AppDriver driver(app);
        driver.start();
        app.setEditMode(true);
        driver.click(*app.find("target"), gran::platform::MouseButton::RIGHT);
        auto* destination = app.find("destination");
        TEST_ASSERT(destination && destination->content.option == "parent");
        TEST_ASSERT(!gran::ui::findOption(*destination, "target") && !gran::ui::findOption(*destination, "descendant"));
        TEST_ASSERT(gran::ui::findOption(*destination, "other"));
        app.selectOption(*destination, "other");
        auto* action = moveButton(*gran::ui::popupOf(destination));
        TEST_ASSERT(action);
        driver.click(*action);
        TEST_ASSERT(app.find("target")->parent == app.find("other"));
    }

    gran::ui::Node& openInspector(gran::Application& app, gran::AppDriver& driver)
    {
        using namespace gran::platform;
        app.setEditMode(true);
        driver.press(Key::TAB);
        auto* button = app.find("button");
        TEST_ASSERT(button && (button->stateMask & gran::style::stateBit(gran::style::State::FOCUSED)));
        driver.press(Key::F10, MODIFIER_SHIFT);
        auto* dialog = gran::ui::popupOf(app.find("token"));
        TEST_ASSERT(dialog && dialog->visible && dialog->kind == gran::ui::ElementKind::DIALOG);
        TEST_ASSERT(driver.windowCount() == 1);
        return *dialog;
    }

    gran::ui::Node& typePreview(gran::Application& app, gran::AppDriver& driver)
    {
        auto* token = app.find("token");
        auto* tab = token->parent;
        while (tab && tab->kind != gran::ui::ElementKind::TAB)
            tab = tab->parent;
        TEST_ASSERT(tab && gran::ui::headerOfPage(*tab));
        driver.click(*gran::ui::headerOfPage(*tab));
        app.selectOption(*token, "size");
        auto* value = app.find("value");
        TEST_ASSERT(value && value->layout.frame.width > 0);
        driver.click(*value);
        driver.type("18");
        return *value;
    }

    void testPreviewPreservesInspector()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window><Resources><Theme><Token name="size" value="12" type="size"/></Theme></Resources>
            <Stack id="root"><Button id="button" text="Editable" onFocus="recordFocus" customizable="style" fontSize="$size"/>
            <Text id="peer" fontSize="$size">Peer</Text><Stack id="runtimeHost"/></Stack></Window>)");
        int focusEvents = 0;
        app.onAction("recordFocus", [&](gran::ui::Node&) { ++focusEvents; });
        auto draft = std::make_unique<gran::ui::Node>();
        draft->id = "draft";
        auto* keptDraft = &app.append(*app.find("runtimeHost"), std::move(draft));
        gran::AppDriver driver(app);
        driver.start();
        auto* dialog = &openInspector(app, driver);
        const int beforePreview = focusEvents;
        TEST_ASSERT(beforePreview == 1);
        auto* value = &typePreview(app, driver);
        TEST_ASSERT(focusEvents == beforePreview);
        TEST_ASSERT(app.find("value") == value && gran::ui::popupOf(value) == dialog && dialog->visible);
        TEST_ASSERT(value->content.text == "18" && app.find("token")->content.option == "size");
        TEST_ASSERT(value->stateMask & gran::style::stateBit(gran::style::State::FOCUSED));
        TEST_ASSERT(app.find("draft") == keptDraft);
        TEST_ASSERT_NEAR(app.find("button")->computed.fontSize, 18.0f, 0.001f);
        TEST_ASSERT_NEAR(app.find("peer")->computed.fontSize, 12.0f, 0.001f);
        driver.press(gran::platform::Key::ESCAPE);
        auto* closed = gran::ui::popupOf(app.find("value"));
        TEST_ASSERT(!closed || !closed->visible);
        TEST_ASSERT_NEAR(app.find("button")->computed.fontSize, 12.0f, 0.001f);
        TEST_ASSERT(app.find("button")->stateMask & gran::style::stateBit(gran::style::State::FOCUSED));
        TEST_ASSERT_EQ(std::to_string(focusEvents), std::to_string(beforePreview + 1));
        TEST_ASSERT(app.profiles().empty() && app.find("draft") == keptDraft);
    }

    void testGlobalTokenWithoutCustomization()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window><Resources><Theme><Token name="size" value="12" type="size"/></Theme></Resources>
            <Stack><Button id="button" text="Button" fontSize="$size"/>
            <Text id="peer" fontSize="$size">Peer</Text></Stack></Window>)");
        gran::AppDriver driver(app);
        driver.start();
        openInspector(app, driver);
        auto& value = typePreview(app, driver);
        TEST_ASSERT(app.find("scope")->content.option == "global");
        for (const auto& control : value.parent->children)
            if (control->content.onClick == "gran:layout:style")
            {
                driver.click(*control);
                break;
            }
        TEST_ASSERT(app.profiles() == std::vector<std::string>{"default"});
        const auto xml = app.exportProfile("default");
        TEST_ASSERT(xml.find("id=") == std::string::npos);
        TEST_ASSERT_NEAR(app.find("button")->computed.fontSize, 18.0f, 0.001f);
        TEST_ASSERT_NEAR(app.find("peer")->computed.fontSize, 18.0f, 0.001f);
    }

}

namespace suites
{
    void registerLayoutPreviewTests(test::TestRunner& runner)
    {
        runner.add("layout: keyboard inspector preserves fields during preview and cancels on Escape",
            testPreviewPreservesInspector);
        runner.add("layout: pointer moves a button into an editable container", layout_tests::testPointerMovesButton);
        runner.add("layout: a middle click does not close a tab while editing", layout_tests::testMiddleClickDoesNotCloseTabWhileEditing);
        runner.add("layout: pointer reorders a vertical tab bar", layout_tests::testPointerReordersVerticalTabBar);
        runner.add("layout: inspector excludes the edited subtree from move destinations", testInspectorMoveDestinations);
        runner.add("layout: applies an exposed global token without customization rights", testGlobalTokenWithoutCustomization);
    }
}
