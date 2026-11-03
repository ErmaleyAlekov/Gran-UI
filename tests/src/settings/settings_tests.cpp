#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/settings/settings.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{
    // Путь файла настроек в чистом временном каталоге / The settings file path in a clean temporary directory
    std::string freshPath(const char* name)
    {
        std::filesystem::path folder = std::filesystem::temp_directory_path() / name;
        std::error_code error;

        std::filesystem::remove_all(folder, error);
        return (folder / "settings.xml").u8string();
    }

    void testValues()
    {
        gran::settings::Settings settings;

        settings.setString("name", "Склад");
        settings.setNumber("zoom", 1.25);
        settings.setBool("dark", true);
        settings.setList("recent", {"a.txt", "b.txt"});
        TEST_ASSERT(settings.modified());
        TEST_ASSERT(settings.getString("name") == "Склад" && settings.getString("zoom") == "1.25");
        TEST_ASSERT(settings.getNumber("zoom") == 1.25 && settings.getNumber("name", 7.0) == 7.0);
        TEST_ASSERT(settings.getBool("dark") && !settings.getBool("name"));
        TEST_ASSERT((settings.getList("recent") == std::vector<std::string>{"a.txt", "b.txt"}));
        TEST_ASSERT(settings.getString("recent", "-") == "-" && settings.getList("name").empty());
        TEST_ASSERT(settings.find("dark") != nullptr && settings.find("dark")->type == gran::settings::ValueType::BOOLEAN);
        TEST_ASSERT(settings.remove("name") && !settings.remove("name") && settings.find("name") == nullptr);
    }

    void testRoundTrip()
    {
        gran::settings::Settings settings;
        gran::settings::Settings loaded;
        gran::platform::WindowPlacement placement{-8, 30, 1200, 800, true};
        gran::platform::WindowPlacement read;
        gran::settings::Value variable;
        std::string value;
        std::string path = freshPath("gran_settings_round");

        TEST_ASSERT(settings.load(path) && settings.path() == path && !settings.modified());
        settings.setString("text", "a & b < \"c\"\nвторая строка");
        settings.setList("empty", {});
        settings.setWindowPlacement("main", placement);
        settings.setAttribute({"main", "side", "width"}, "280");
        settings.setVariable("dark", gran::settings::Value{gran::settings::ValueType::BOOLEAN, "true", {}});
        TEST_ASSERT(settings.save() && !settings.modified());
        TEST_ASSERT(loaded.load(path));
        TEST_ASSERT(loaded.getString("text") == "a & b < \"c\"\nвторая строка");
        TEST_ASSERT(loaded.find("empty") != nullptr && loaded.getList("empty").empty());
        TEST_ASSERT(loaded.windowPlacement("main", read) && read.x == -8 && read.width == 1200 && read.maximized);
        TEST_ASSERT(loaded.attribute({"main", "side", "width"}, value) && value == "280");
        TEST_ASSERT(loaded.variable("dark", variable) && variable.text == "true");
        loaded.setWindowPlacement("main", read);
        TEST_ASSERT(!loaded.modified());
    }

    void testDamagedFile()
    {
        gran::settings::Settings settings;
        std::string path = freshPath("gran_settings_damaged");
        std::filesystem::path file = std::filesystem::u8path(path);
        std::filesystem::path damaged = file;
        std::error_code error;

        damaged += ".bad";
        std::filesystem::create_directories(file.parent_path(), error);
        std::ofstream(file) << "<Settings><Value name=\"x\" type=\"number\">не число</Value></Settings>";
        TEST_ASSERT(!settings.load(path));
        TEST_ASSERT(std::filesystem::exists(damaged) && !std::filesystem::exists(file));
        TEST_ASSERT(settings.find("x") == nullptr);
        std::ofstream(file) << "<Settings><Value name=\"y\" type=\"boolean\">да</Value></Settings>";
        TEST_ASSERT(!settings.load(path));
        TEST_ASSERT(settings.find("y") == nullptr);
        std::ofstream(file) << "<Settings><Value name=\"x\"";
        TEST_ASSERT(!settings.load(path));
        TEST_ASSERT(gran::settings::defaultPath("App").find("settings.xml") != std::string::npos);
    }
}

namespace suites
{
    void registerSettingsTests(test::TestRunner& runner)
    {
        runner.add("settings: typed values", testValues);
        runner.add("settings: save and load round trip", testRoundTrip);
        runner.add("settings: damaged file goes to .bad", testDamagedFile);
    }
}
