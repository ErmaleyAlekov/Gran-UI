#include "framework/test_framework.hpp"
#include "gran/app/app_driver.hpp"

namespace layout_tests
{
    namespace
    {
        constexpr auto MARKUP = R"(<Window height="850"><Resources>
            <Variable name="captionText" value="Paste and save"/>
            <Command id="create" key="Mod+Shift+F11" action="gran:createElement"/>
            <Command id="user-1"/><Command id="user-2.command"/>
            <Command id="paste" text="Paste" action="gran:paste"/><Command id="save" text="Save" action="save"/>
            <Palette><CommandRef ref="paste"/><CommandRef ref="save"/>
            <Template name="tool"><Button text="{$captionText}" customizable="move command"/></Template>
            </Palette></Resources><Stack><Input id="document"/>
            <Stack id="locked" height="60"/><Stack id="tools" customizable="insert" height="100"/>
            </Stack></Window>)";

        gran::ui::Node* action(gran::ui::Node& node, const std::string& name)
        {
            if (node.content.onClick == name) return &node;
            for (auto& child : node.children)
                if (auto* found = action(*child, name)) return found;
            return nullptr;
        }

        void openCreator(gran::Application& app, gran::AppDriver& driver)
        {
            using namespace gran::platform;
            app.setEditMode(true);
            driver.click(*app.find("locked"));
            driver.press(Key::F11, MODIFIER_CONTROL | MODIFIER_SHIFT);
            TEST_ASSERT(!app.find("actions"));
            driver.click(*app.find("tools"));
            driver.press(Key::F11, MODIFIER_CONTROL | MODIFIER_SHIFT);
            TEST_ASSERT(app.find("actions") && app.find("actions")->visible);
            TEST_ASSERT(app.find("source")->content.option == "t:tool" && driver.windowCount() == 1);
        }
    }

    void testCreateLiveButton()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(MARKUP);
        std::string saved;
        app.onAction("save", [&](gran::ui::Node&) { saved = app.find("document")->content.text; });
        gran::AppDriver driver(app);
        driver.start();
        driver.click(*app.find("document"));
        driver.type("abc");
        driver.press(gran::platform::Key::LEFT, gran::platform::MODIFIER_SHIFT);
        openCreator(app, driver);
        auto* dialog = gran::ui::popupOf(app.find("source"));
        auto* step = action(*dialog, "gran:layout:step");
        auto* create = action(*dialog, "gran:layout:create");
        TEST_ASSERT(step && create);
        driver.click(*step);
        app.selectOption(*app.find("action"), "c:save");
        driver.click(*step);
        driver.click(*create);
        TEST_ASSERT(app.find("user-3") && app.find("user-3")->parent == app.find("tools"));
        TEST_ASSERT(app.find("user-3")->content.text == "Paste and save");
        app.setEditMode(false);
        driver.type("X");
        TEST_ASSERT(app.find("document")->content.text == "abX");
        driver.window().setClipboardText("draft");
        driver.click(*app.find("document"));
        driver.press(gran::platform::Key::A, gran::platform::MODIFIER_CONTROL);
        driver.click(*app.find("user-3"));
        TEST_ASSERT(saved == "draft");
    }
}
