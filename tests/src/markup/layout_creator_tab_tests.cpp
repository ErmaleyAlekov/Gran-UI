#include "framework/test_framework.hpp"
#include "gran/app/app_driver.hpp"

namespace layout_tests
{
    namespace
    {
        void createPage(gran::Application& app, gran::AppDriver& driver)
        {
            using namespace gran::platform;
            driver.click(*gran::ui::headerOfPage(*app.find("files")));
            driver.press(Key::UP, MODIFIER_ALT);
            driver.press(Key::F11, MODIFIER_CONTROL | MODIFIER_SHIFT);
            auto* source = app.find("source");
            TEST_ASSERT(source && source->content.option == "t:page");
            TEST_ASSERT(!gran::ui::findOption(*source, "t:field"));
            auto* dialog = gran::ui::popupOf(source);
            for (const auto& child : dialog->children)
            {
                if (child->content.onClick == "gran:layout:create")
                {
                    driver.click(*child);
                    return;
                }
            }
            TEST_ASSERT(false);
        }

    }

    void drag(gran::AppDriver& driver, gran::Vec2 from, gran::Vec2 to)
    {
        using namespace gran::platform;
        for (auto kind : {EventKind::MOUSE_DOWN, EventKind::MOUSE_MOVE, EventKind::MOUSE_UP})
        {
            Event event;
            event.kind = kind;
            event.position = kind == EventKind::MOUSE_DOWN ? from : to;
            driver.window().pushEvent(event);
            driver.settle();
        }
    }

    void testCreateAndMoveTabs()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window><Resources><Command id="create" key="Mod+Shift+F11" action="gran:createElement"/>
            <Palette><Template name="page"><Tab text="Page" customizable="move insert"/></Template>
            <Template name="field"><Input customizable="style"/></Template></Palette></Resources>
            <Tabs id="tabs" customizable="insert"><Tab id="files" text="Files" customizable="insert"/></Tabs></Window>)");
        gran::AppDriver driver(app);
        driver.start();
        app.setEditMode(true);
        createPage(app, driver);
        createPage(app, driver);
        auto* first = app.find("user-1");
        auto* second = app.find("user-2");
        TEST_ASSERT(first && second);
        auto* header = gran::ui::headerOfPage(*first);
        auto before = driver.centerOf(*header);
        before.x -= header->layout.frame.width / 4;
        drag(driver, driver.centerOf(*gran::ui::headerOfPage(*second)), before);
        TEST_ASSERT(gran::ui::headerNumber(*gran::ui::headerOfPage(*app.find("user-2"))) == 1);
        app.setEditMode(false);
        driver.click(*gran::ui::headerOfPage(*app.find("user-2")));
        TEST_ASSERT(app.find("user-2")->visible && !app.find("user-1")->visible);
    }
}
