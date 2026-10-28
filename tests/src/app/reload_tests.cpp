#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/core/gran_exception.hpp"
#include <fstream>
#include <filesystem>

namespace
{
    void testLoadStringReplaceTree()
    {
        gran::Application app;
        app.loadString(R"(<Window><Text id="old">Old</Text></Window>)");
        auto* oldNode = app.find("old");
        TEST_ASSERT(oldNode != nullptr);
        app.focus(oldNode);
        app.loadString(R"(<Window><Text id="new">New</Text></Window>)");
        auto* newNode = app.find("new");
        TEST_ASSERT(newNode != nullptr);
        TEST_ASSERT(app.find("old") == nullptr);
        TEST_ASSERT_THROWS(app.loadString("<Window>"), gran::MarkupException);
        TEST_ASSERT(app.find("new") == newNode);
        TEST_ASSERT_THROWS(app.loadString("<Window><Resources><Font family=\"Missing\" source=\"missing-reload.ttf\"/></Resources><Text/></Window>"), gran::GranException);
        TEST_ASSERT(app.find("new") == newNode);
    }

    void testReloadWithoutLoadFile()
    {
        gran::Application app;
        TEST_ASSERT_THROWS(app.reload(), gran::GranException);
    }

    void testReloadIfChanged()
    {
        const std::string tempFilePath = "scratchpad/reload_unit.xml";
        std::filesystem::create_directories("scratchpad");
        std::ofstream tempFile(tempFilePath);
        tempFile << R"(<Window><Text id="initial">Initial</Text></Window>)";
        tempFile.close();

        gran::Application app;
        app.loadFile(tempFilePath);
        TEST_ASSERT(app.find("initial") != nullptr);

        TEST_ASSERT(!app.reloadIfChanged());

        std::ofstream tempFile2(tempFilePath);
        tempFile2 << R"(<Window><Text id="changed">Changed content longer</Text></Window>)";
        tempFile2.close();

        TEST_ASSERT(app.reloadIfChanged());
        TEST_ASSERT(app.find("changed") != nullptr);
        TEST_ASSERT(app.find("initial") == nullptr);

        TEST_ASSERT(!app.reloadIfChanged());

        std::filesystem::remove(tempFilePath);
    }

    void testReloadInvalidFile()
    {
        const std::string tempFilePath = "scratchpad/reload_unit.xml";
        std::filesystem::create_directories("scratchpad");
        std::ofstream tempFile(tempFilePath);
        tempFile << R"(<Window><Text id="invalid">Invalid</Text></Window>)";
        tempFile.close();

        gran::Application app;
        app.loadFile(tempFilePath);
        TEST_ASSERT(app.find("invalid") != nullptr);
        std::ofstream broken(tempFilePath);
        broken << "<Window>";
        broken.close();
        TEST_ASSERT_THROWS(app.reload(), gran::MarkupException);
        TEST_ASSERT(app.find("invalid") != nullptr);

        std::ofstream tempFile2(tempFilePath);
        tempFile2 << R"(<Window><Text id="valid">Valid</Text></Window>)";
        tempFile2.close();

        app.reload();
        TEST_ASSERT(app.find("valid") != nullptr);
        TEST_ASSERT(app.find("invalid") == nullptr);

        app.loadString(R"(<Window><Text id="final">Final</Text></Window>)");
        TEST_ASSERT(app.find("final") != nullptr);
        TEST_ASSERT(app.find("valid") == nullptr);
        TEST_ASSERT_THROWS(app.reload(), gran::GranException);

        std::filesystem::remove(tempFilePath);
    }
}

namespace suites
{
    void registerReloadTests(test::TestRunner& runner)
    {
        runner.add("loadString: replace tree", testLoadStringReplaceTree);
        runner.add("reload: without loadFile", testReloadWithoutLoadFile);
        runner.add("reloadIfChanged: basic functionality", testReloadIfChanged);
        runner.add("reload: invalid file", testReloadInvalidFile);
    }
}
