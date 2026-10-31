#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/core/gran_exception.hpp"
#include <memory>
#include <optional>
#include <string>
#include <filesystem>
#include <fstream>

namespace
{
    void testLiveAppSetAttribute()
    {
        gran::Application app;
        app.loadString(R"(<Window><Stack id="panel" padding="4"><Input id="name"/><Text id="label">A</Text></Stack></Window>)");
        auto* panel = app.find("panel");
        TEST_ASSERT(panel != nullptr);
        app.find("name")->content.text = "typed";
        app.setMarkupAttribute(*panel, "padding", "8");
        TEST_ASSERT(app.find("panel") == panel);
        TEST_ASSERT(*app.markupAttribute(*panel, "padding") == "8");
        TEST_ASSERT(app.find("name") != nullptr);
        TEST_ASSERT(app.find("name")->content.text == "typed");
    }

    void testLiveAppSetAttributeOnChild()
    {
        gran::Application app;
        app.loadString(R"(<Window><Stack id="panel" padding="4"><Input id="name"/><Text id="label">A</Text></Stack></Window>)");
        app.find("name")->content.text = "typed";
        app.setMarkupAttribute(*app.find("name"), "value", "fresh");
        TEST_ASSERT(app.find("name")->content.text == "fresh");
    }

    void testLiveAppSetInvalidAttribute()
    {
        gran::Application app;
        app.loadString(R"(<Window><Stack id="panel" padding="4"><Input id="name"/><Text id="label">A</Text></Stack></Window>)");
        auto* panel = app.find("panel");
        TEST_ASSERT_THROWS(app.setMarkupAttribute(*panel, "padding", "abc"), gran::MarkupException);
        TEST_ASSERT(*app.markupAttribute(*panel, "padding") == "4");
        app.removeMarkupAttribute(*panel, "padding");
        TEST_ASSERT(!app.markupAttribute(*panel, "padding").has_value());
    }

    void testLiveAppSetAttributeOnCodeNode()
    {
        gran::Application app;
        app.loadString(R"(<Window><Stack id="panel" padding="4"><Input id="name"/><Text id="label">A</Text></Stack></Window>)");
        auto& added = app.append(*app.find("panel"), std::make_unique<gran::ui::Node>());
        TEST_ASSERT_THROWS(app.setMarkupAttribute(added, "padding", "1"), gran::GranException);
        TEST_ASSERT(!app.markupAttribute(added, "padding").has_value());
    }

    // Узел из кода переживает перестройку родителя из разметки / A node from code survives the parent rebuild from the markup
    void testLiveAppRuntimeChildKept()
    {
        gran::Application app;
        app.loadString(R"(<Window><Stack id="panel" padding="4"><Text id="label">A</Text></Stack></Window>)");
        gran::ui::Node* panel = app.find("panel");
        gran::ui::Node& added = app.append(*panel, std::make_unique<gran::ui::Node>());
        app.setMarkupAttribute(*panel, "padding", "8");
        TEST_ASSERT(panel->children.size() == 2 && panel->children.back().get() == &added);
        TEST_ASSERT(added.parent == panel && app.find("label") != nullptr);
    }

    void testLiveAppSaveMarkup()
    {
        gran::Application app;
        app.loadString(R"(<Window><Stack id="panel" padding="4"><Input id="name"/><Text id="label">A</Text></Stack></Window>)");
        app.appendMarkup(*app.find("panel"), R"(<Button id="extra">Go</Button>)");
        app.remove(*app.find("label"));
        app.setMarkupAttribute(*app.find("panel"), "padding", "8");
        std::string xml = app.saveMarkup();
        TEST_ASSERT(xml.find("extra") != std::string::npos);
        TEST_ASSERT(xml.find("padding=\"8\"") != std::string::npos);
        TEST_ASSERT(xml.find("label") == std::string::npos);

        gran::Application second;
        second.loadString(xml);
        TEST_ASSERT(second.find("extra") != nullptr);
        TEST_ASSERT(second.find("label") == nullptr);
    }

    void testLiveAppRichTextPersistence()
    {
        gran::Application app;
        app.loadString(R"(<Window><Text id="rich">Hello <Span fontWeight="700">big</Span> world</Text></Window>)");
        std::string before = app.find("rich")->content.text;
        gran::Application second;
        second.loadString(app.saveMarkup());
        TEST_ASSERT(second.find("rich")->content.text == before);
    }

    void testLiveAppPartialReload()
    {
        const std::string tempFilePath = "scratchpad/live_reload.xml";
        std::filesystem::create_directories("scratchpad");
        std::ofstream tempFile(tempFilePath);
        tempFile << R"(<Window><Stack id="root"><Stack id="left"><Input id="field"/></Stack><Text id="right">One</Text></Stack></Window>)";
        tempFile.close();

        gran::Application app;
        app.loadFile(tempFilePath);
        auto* left = app.find("left");
        auto* field = app.find("field");
        TEST_ASSERT(left != nullptr);
        TEST_ASSERT(field != nullptr);
        field->content.text = "typed";

        std::ofstream tempFile2(tempFilePath);
        tempFile2 << R"(<Window><Stack id="root"><Stack id="left"><Input id="field"/></Stack><Text id="right">Two</Text></Stack></Window>)";
        tempFile2.close();

        app.reload();
        TEST_ASSERT(app.find("left") == left);
        TEST_ASSERT(app.find("field") == field);
        TEST_ASSERT(app.find("field")->content.text == "typed");
        TEST_ASSERT(app.find("right")->content.text == "Two");

        std::filesystem::remove(tempFilePath);
    }

    void testLiveAppFullReloadWithState()
    {
        const std::string tempFilePath = "scratchpad/live_reload.xml";
        std::filesystem::create_directories("scratchpad");
        std::ofstream tempFile(tempFilePath);
        tempFile << R"(<Window title="A"><Stack id="root"><Stack id="left"><Input id="field"/></Stack><Text id="right">One</Text></Stack></Window>)";
        tempFile.close();

        gran::Application app;
        app.loadFile(tempFilePath);
        auto* field = app.find("field");
        TEST_ASSERT(field != nullptr);
        field->content.text = "typed";

        std::ofstream tempFile2(tempFilePath);
        tempFile2 << R"(<Window title="B"><Stack id="root"><Stack id="left"><Input id="field"/></Stack><Text id="right">Two</Text></Stack></Window>)";
        tempFile2.close();

        app.reload();
        TEST_ASSERT(app.find("field")->content.text == "typed");
        TEST_ASSERT(app.find("right")->content.text == "Two");

        std::filesystem::remove(tempFilePath);
    }
}

namespace suites
{
    void registerLiveAppTests(test::TestRunner& runner)
    {
        runner.add("live: set attribute", testLiveAppSetAttribute);
        runner.add("live: set attribute on child", testLiveAppSetAttributeOnChild);
        runner.add("live: set invalid attribute", testLiveAppSetInvalidAttribute);
        runner.add("live: set attribute on code node", testLiveAppSetAttributeOnCodeNode);
        runner.add("live: runtime child kept", testLiveAppRuntimeChildKept);
        runner.add("live: save markup", testLiveAppSaveMarkup);
        runner.add("live: rich text persistence", testLiveAppRichTextPersistence);
        runner.add("live: partial reload", testLiveAppPartialReload);
        runner.add("live: full reload with state", testLiveAppFullReloadWithState);
    }
}
