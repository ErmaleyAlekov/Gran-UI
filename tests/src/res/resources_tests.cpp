#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/markup/markup_loader.hpp"
#include "gran/res/manifest.hpp"
#include "gran/res/resources.hpp"
#include "gran/utils/mapped_file.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{
    // Пакет теста: окно с картинкой рядом и данные / The test bundle: a window with an image beside it and data
    const unsigned char WINDOW[] = "<Window title=\"R\"><Text id=\"t\">res</Text></Window>";
    const unsigned char DATA[] = "0123456789";
    const gran::res::Entry ENTRIES[] = {
        {"ui/main.xml", WINDOW, sizeof(WINDOW) - 1},
        {"data/digits.txt", DATA, sizeof(DATA) - 1}};
    const gran::res::Bundle BUNDLE{"test", ENTRIES, 2};

    void testNormalizeAndFind()
    {
        gran::res::ResourceView view;

        TEST_ASSERT(gran::res::normalize("res:/a/./b/../c.png") == "a/c.png");
        TEST_ASSERT(gran::res::normalize("res:\\ui\\x\\..\\main.xml") == "ui/main.xml");
        TEST_ASSERT(gran::res::normalize("res:/../outside").empty());
        TEST_ASSERT(gran::res::isResourcePath("res:/a") && gran::res::isResourcePath("res:\\a"));
        TEST_ASSERT(!gran::res::isResourcePath("resources/a") && !gran::res::isResourcePath("C:/a"));
        gran::res::unmountAll();
        gran::res::mount(BUNDLE);
        TEST_ASSERT(gran::res::find("res:/ui\\main.xml", view) && view.size == sizeof(WINDOW) - 1);
        TEST_ASSERT(!gran::res::find("res:/ui/other.xml", view));
        TEST_ASSERT(!gran::res::find("ui/main.xml", view));
    }

    void testStreamsAndFiles()
    {
        std::vector<std::uint8_t> bytes;
        std::string text;

        gran::res::mount(BUNDLE);
        std::unique_ptr<std::istream> stream = gran::res::openStream("res:/data/digits.txt");
        stream->seekg(4);
        *stream >> text;
        TEST_ASSERT(text == "456789");
        stream->clear();
        stream->seekg(-3, std::ios::end);
        TEST_ASSERT(stream->get() == '7');
        TEST_ASSERT(!*gran::res::openStream("res:/missing.txt"));
        TEST_ASSERT(gran::res::readFile("res:/data/digits.txt", bytes) && bytes.size() == 10);
        TEST_ASSERT(!gran::res::readFile("res:/missing.txt", bytes));
        std::shared_ptr<const gran::MappedFile> mapped = gran::MappedFile::open("res:/data/digits.txt");
        TEST_ASSERT(mapped && mapped->size() == 10 && mapped->data() == DATA);
    }

    void testMarkupFromResources()
    {
        gran::res::mount(BUNDLE);
        gran::markup::UiDocument document = gran::markup::loadMarkupFile("res:/ui/main.xml", gran::markup::LoadOptions{});
        TEST_ASSERT(document.window.title == "R");
        TEST_ASSERT(document.root && document.root->content.text == "res");
        TEST_ASSERT(gran::res::isResourcePath(document.baseDirectory));
    }

    void testBlobRoundTrip()
    {
        std::filesystem::path folder = std::filesystem::temp_directory_path() / "gran_res_manifest";
        std::error_code error;
        gran::res::ResourceView view;

        std::filesystem::remove_all(folder, error);
        std::filesystem::create_directories(folder / "assets" / "deep", error);
        std::ofstream(folder / "main.xml") << "<Window><Text>m</Text></Window>";
        std::ofstream(folder / "assets" / "a.svg") << "svg";
        std::ofstream(folder / "assets" / "b.txt") << "skip";
        std::ofstream(folder / "assets" / "deep" / "c.svg") << "deep";
        std::ofstream(folder / "bundle.xml") << "<Bundle name=\"probe\"><File path=\"main.xml\" alias=\"ui/main.xml\"/>"
            "<Folder path=\"assets\" alias=\"icons\" include=\"*.svg\"/></Bundle>";
        gran::res::Manifest manifest = gran::res::readManifest(folder / "bundle.xml");
        TEST_ASSERT(manifest.name == "probe" && manifest.files.size() == 3);
        TEST_ASSERT(manifest.files[0].alias == "icons/a.svg" && manifest.files[1].alias == "icons/deep/c.svg");
        std::string blob = gran::res::generateBlob(manifest);
        gran::res::mountBlob(blob.data(), blob.size());
        TEST_ASSERT(gran::res::find("res:/icons/deep/c.svg", view) && view.size == 4);
        TEST_ASSERT(gran::res::generateSource(manifest).find("granResources_probe()") != std::string::npos);
        TEST_ASSERT(gran::res::generateDepfile(manifest, "out.cpp").find("main.xml") != std::string::npos);
        TEST_ASSERT_THROWS(gran::res::mountBlob(blob.data(), 12), gran::GranException);
    }

    void testManifestErrors()
    {
        std::filesystem::path folder = std::filesystem::temp_directory_path() / "gran_res_errors";
        std::error_code error;

        std::filesystem::create_directories(folder, error);
        std::ofstream(folder / "one.txt") << "1";
        std::ofstream(folder / "bad.xml") << "<Bundle name=\"9bad\"/>";
        std::ofstream(folder / "missing.xml") << "<Bundle name=\"m\"><File path=\"nope.txt\"/></Bundle>";
        std::ofstream(folder / "twice.xml") << "<Bundle name=\"t\"><File path=\"one.txt\" alias=\"x/one.txt\"/>"
            "<Folder path=\".\" alias=\"x\" include=\"one.txt\" recursive=\"false\"/></Bundle>";
        TEST_ASSERT_THROWS(gran::res::readManifest(folder / "bad.xml"), gran::GranException);
        TEST_ASSERT_THROWS(gran::res::readManifest(folder / "missing.xml"), gran::GranException);
        TEST_ASSERT_THROWS(gran::res::readManifest(folder / "twice.xml"), gran::GranException);
        TEST_ASSERT(gran::res::matchPattern("*.s?g", "icon.svg") && !gran::res::matchPattern("*.png", "icon.svg"));
        TEST_ASSERT(gran::res::matchPattern("*", "") && gran::res::matchPattern("a*b*c", "aXbYc"));
        gran::res::unmountAll();
    }
}

namespace suites
{
    void registerResourceTests(test::TestRunner& runner)
    {
        runner.add("res: normalize and find", testNormalizeAndFind);
        runner.add("res: streams, files, mapped file", testStreamsAndFiles);
        runner.add("res: markup from res:/", testMarkupFromResources);
        runner.add("res: manifest and blob round trip", testBlobRoundTrip);
        runner.add("res: manifest errors and patterns", testManifestErrors);
    }
}
