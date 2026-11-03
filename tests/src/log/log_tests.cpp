#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/log/crash_report.hpp"
#include "gran/log/log.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

namespace
{
    // Свежий временный каталог теста / A fresh temporary test directory
    std::filesystem::path freshFolder(const char* name)
    {
        std::filesystem::path folder = std::filesystem::temp_directory_path() / name;
        std::error_code error;

        std::filesystem::remove_all(folder, error);
        std::filesystem::create_directories(folder, error);
        return folder;
    }

    // Текст файла целиком / The whole file text
    std::string readText(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        std::ostringstream text;

        text << input.rdbuf();
        return text.str();
    }

    void testWriteAndLevel()
    {
        gran::log::LogOptions options;
        std::filesystem::path folder = freshFolder("gran_log_write");

        options.appName = "LogTest";
        options.directory = folder.u8string();
        TEST_ASSERT(gran::log::open(options));
        TEST_ASSERT(gran::log::open(options));
        gran::log::write(gran::log::Level::INFO, "unit", "строка журнала");
        gran::log::write(gran::log::Level::VERBOSE, "unit", "hidden detail");
        std::string path = gran::log::filePath();
        TEST_ASSERT(std::filesystem::u8path(path) == folder / "LogTest.log");
        TEST_ASSERT(gran::log::directory() == folder.u8string());
        std::string text = readText(std::filesystem::u8path(path));
        TEST_ASSERT(text.find("| INFO    | ") != std::string::npos);
        TEST_ASSERT(text.find("| unit | строка журнала\n") != std::string::npos);
        TEST_ASSERT(text.find("hidden detail") == std::string::npos);
        gran::log::close();
        TEST_ASSERT(gran::log::filePath().empty());
    }

    void testRotation()
    {
        gran::log::LogOptions options;
        std::filesystem::path folder = freshFolder("gran_log_rotate");

        options.appName = "Rotate";
        options.directory = folder.u8string();
        options.maxBytes = 300;
        options.files = 3;
        TEST_ASSERT(gran::log::open(options));
        for (int index = 0; index < 30; ++index)
            gran::log::write(gran::log::Level::INFO, "rotate", "line " + std::to_string(index));
        gran::log::close();
        TEST_ASSERT(std::filesystem::exists(folder / "Rotate.log"));
        TEST_ASSERT(std::filesystem::exists(folder / "Rotate.1.log"));
        TEST_ASSERT(std::filesystem::exists(folder / "Rotate.2.log"));
        TEST_ASSERT(!std::filesystem::exists(folder / "Rotate.3.log"));
        TEST_ASSERT(std::filesystem::file_size(folder / "Rotate.1.log") >= 300);
    }

    void testRecentRing()
    {
        char buffer[256];

        gran::log::write(gran::log::Level::INFO, "ring", "последняя запись");
        std::size_t count = gran::log::copyRecent(buffer, sizeof(buffer));
        std::string tail(buffer, count);
        TEST_ASSERT(count > 0 && count <= sizeof(buffer));
        TEST_ASSERT(tail.find("| ring | последняя запись\n") != std::string::npos);
        TEST_ASSERT(tail.back() == '\n');
    }

    void testPreviousCrash()
    {
        std::filesystem::path folder = freshFolder("gran_log_crashes");
        std::filesystem::path first = folder / "app-1-1.txt";
        std::filesystem::path second = folder / "app-2-2.txt";

        TEST_ASSERT(gran::log::previousCrash(folder.u8string()).empty());
        std::ofstream(first) << "report";
        TEST_ASSERT(std::filesystem::u8path(gran::log::previousCrash(folder.u8string())) == first);
        TEST_ASSERT(gran::log::previousCrash(folder.u8string()).empty());
        std::ofstream(second) << "report";
        std::filesystem::last_write_time(second, std::filesystem::last_write_time(first) + std::chrono::seconds(5));
        TEST_ASSERT(std::filesystem::u8path(gran::log::previousCrash(folder.u8string())) == second);
    }

    void testPaths()
    {
        gran::log::CrashOptions crash;
        std::filesystem::path logs = std::filesystem::u8path(gran::log::defaultDirectory("PathApp"));

        TEST_ASSERT(logs.filename() == "logs" && logs.parent_path().filename() == "PathApp");
        crash.appName = "PathApp";
        TEST_ASSERT(std::filesystem::u8path(gran::log::crashDirectory(crash)) == logs.parent_path() / "crashes");
        crash.directory = "custom";
        TEST_ASSERT(gran::log::crashDirectory(crash) == "custom");
        TEST_ASSERT(!gran::log::systemDescription().empty());
        TEST_ASSERT(!gran::log::open(gran::log::LogOptions{}));
    }
}

namespace suites
{
    void registerLogTests(test::TestRunner& runner)
    {
        runner.add("log: write and level", testWriteAndLevel);
        runner.add("log: rotation", testRotation);
        runner.add("log: recent ring", testRecentRing);
        runner.add("log: previous crash", testPreviousCrash);
        runner.add("log: default paths", testPaths);
    }
}
