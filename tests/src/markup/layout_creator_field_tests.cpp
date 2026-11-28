#include "framework/test_framework.hpp"
#include "gran/app/app_driver.hpp"

namespace layout_tests
{
    void drag(gran::AppDriver& driver, gran::Vec2 from, gran::Vec2 to);

    void testLiveInputMovePolicy()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window><Resources><Palette>
            <Template name="free"><Input customizable="move" width="90" height="35"/></Template>
            <Template name="fixed"><Input customizable="style" width="90" height="35"/></Template>
            </Palette></Resources><Stack orientation="horizontal">
            <Stack id="left" width="120" height="150" customizable="insert"/>
            <Stack id="right" width="120" height="150" customizable="insert"/></Stack></Window>)");
        gran::AppDriver driver(app);
        driver.start();
        app.importProfile("fields", R"(<Overlay version="1"><Insert id="a" parent="left" template="free"/>
            <Insert id="b" parent="left" template="fixed"/></Overlay>)");
        driver.settle();
        driver.click(*app.find("a"));
        driver.type("draft");
        TEST_ASSERT(app.find("a")->content.text == "draft");
        app.setEditMode(true);
        drag(driver, driver.centerOf(*app.find("b")), driver.centerOf(*app.find("right")));
        TEST_ASSERT(app.find("b")->parent == app.find("left"));
        drag(driver, driver.centerOf(*app.find("a")), driver.centerOf(*app.find("right")));
        TEST_ASSERT(app.find("a")->parent == app.find("right"));
        TEST_ASSERT(app.find("a")->content.text == "draft");
        app.selectProfile("fields");
        driver.settle();
        TEST_ASSERT(app.find("a")->parent == app.find("right") && app.find("b")->parent == app.find("left"));
    }
}
