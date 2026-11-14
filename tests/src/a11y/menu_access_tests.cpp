#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/input/input_router.hpp"
#include "gran/markup/markup_loader.hpp"
#include "gran/ui/node.hpp"
#include "support/layout_helpers.hpp"
#include "gran/a11y/access_tree.hpp"
#include "gran/platform/headless/headless_window.hpp"
#include <string>

namespace
{
    class ActionSinkStub : public gran::input::ActionSink
    {
    public:
        void action(const std::string& name, gran::ui::Node&) override
        {
            lastAction = name;
            actionCount++;
        }

        std::string lastAction;
        int actionCount = 0;
    };

    class TextLocatorStub : public gran::input::TextLocator
    {
    public:
        std::size_t offsetAt(const gran::ui::Node&, gran::Vec2) override
        {
            return 0;
        }

        std::size_t lineEdge(const gran::ui::Node&, std::size_t offset, bool) override
        {
            return offset;
        }

        std::size_t lineMove(const gran::ui::Node&, std::size_t offset, int, float&) override
        {
            return offset;
        }

        int pageLines(const gran::ui::Node&) override
        {
            return 1;
        }
    };

    const char* MARKUP = R"(
        <Window>
            <Stack>
                <MenuBar id="bar">
                    <MenuItem id="file" text="_Файл">
                        <Popup id="fileMenu">
                            <MenuItem id="open" text="_Открыть" onClick="open"/>
                            <MenuItem id="under" text="a__b" onClick="under"/>
                        </Popup>
                    </MenuItem>
                    <MenuItem id="edit" text="_Правка">
                        <Popup id="editMenu">
                            <MenuItem id="copy" text="_Копировать" onClick="copy"/>
                        </Popup>
                    </MenuItem>
                    <MenuItem id="view" text="_Вид">
                        <Popup id="viewMenu">
                            <MenuItem id="zoom" text="_Масштаб" onClick="zoom"/>
                        </Popup>
                    </MenuItem>
                </MenuBar>
                <Input id="field" width="200"/>
                <ComboBox id="combo">
                    <MenuItem text="snake_case"/>
                </ComboBox>
            </Stack>
        </Window>
    )";

    void testRolesAndKeys()
    {
        auto doc = support::layoutMarkup(MARKUP, 600, 400);

        TEST_ASSERT(gran::a11y::roleOf(*gran::ui::findNode(*doc.root, "bar")) == gran::a11y::Role::MENU_BAR);
        TEST_ASSERT(gran::a11y::accessKeyOf(*gran::ui::findNode(*doc.root, "file")) == u8"Alt+Ф");
        TEST_ASSERT(gran::a11y::accessKeyOf(*gran::ui::findNode(*doc.root, "open")) == u8"О");
        TEST_ASSERT(gran::a11y::accessKeyOf(*gran::ui::findNode(*doc.root, "under")).empty());
        TEST_ASSERT(gran::a11y::describe(*gran::ui::findNode(*doc.root, "file")).accessKey == u8"Alt+Ф");
    }

    void testExpandPattern()
    {
        auto doc = support::layoutMarkup(MARKUP, 600, 400);

        TEST_ASSERT((gran::a11y::patternsOf(*gran::ui::findNode(*doc.root, "file")) & gran::a11y::PATTERN_EXPAND) != 0);
        TEST_ASSERT((gran::a11y::patternsOf(*gran::ui::findNode(*doc.root, "open")) & gran::a11y::PATTERN_EXPAND) == 0);
        TEST_ASSERT((gran::a11y::patternsOf(*gran::ui::findNode(*doc.root, "open")) & gran::a11y::PATTERN_INVOKE) != 0);
    }

    void testExpandRequest()
    {
        auto doc = support::layoutMarkup(MARKUP, 600, 400);

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);
        auto* editMenu = gran::ui::findNode(*doc.root, "editMenu");

        TEST_ASSERT(router.perform(gran::a11y::AccessRequest{gran::a11y::Request::EXPAND, gran::ui::findNode(*doc.root, "edit"), std::string(), 0.0f}) == true);
        TEST_ASSERT(editMenu->visible);

        TEST_ASSERT(router.perform(gran::a11y::AccessRequest{gran::a11y::Request::EXPAND, gran::ui::findNode(*doc.root, "edit"), std::string(), 0.0f}) == false);

        TEST_ASSERT(router.perform(gran::a11y::AccessRequest{gran::a11y::Request::COLLAPSE, gran::ui::findNode(*doc.root, "edit"), std::string(), 0.0f}) == true);
        TEST_ASSERT(!editMenu->visible);
    }
}

namespace suites
{
    void registerMenuAccessTests(test::TestRunner& runner)
    {
        runner.add("menu access: roles and keys", testRolesAndKeys);
        runner.add("menu access: expand pattern", testExpandPattern);
        runner.add("menu access: expand request", testExpandRequest);
    }
}
