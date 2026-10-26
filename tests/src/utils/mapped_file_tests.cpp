#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/utils/mapped_file.hpp"
#include <filesystem>
#include <fstream>
#include <string>
#include <cstring> // for std::memcmp

namespace
{
    void testOpenExistingFile()
    {
        auto file = gran::MappedFile::open("schemas/gran-ui.xsd");
        TEST_ASSERT(file != nullptr);
        TEST_ASSERT(file->size() == std::filesystem::file_size("schemas/gran-ui.xsd"));
        TEST_ASSERT(std::memcmp(file->data(), "<?xml", 5) == 0);
    }

    void testOpenNonExistentFile()
    {
        auto file = gran::MappedFile::open("non_existent_file.xsd");
        TEST_ASSERT(file == nullptr);
    }

    void testOpenEmptyFile()
    {
        auto tempPath = std::filesystem::temp_directory_path() / "empty_file.txt";
        std::ofstream(tempPath, std::ios::binary).close();

        auto file = gran::MappedFile::open(tempPath.string());
        TEST_ASSERT(file == nullptr);

        std::filesystem::remove(tempPath);
    }

    void testOpenSameFileTwice()
    {
        auto file1 = gran::MappedFile::open("schemas/gran-ui.xsd");
        auto file2 = gran::MappedFile::open("schemas/gran-ui.xsd");

        TEST_ASSERT(file1 != nullptr);
        TEST_ASSERT(file2 != nullptr);
        TEST_ASSERT(std::memcmp(file1->data(), file2->data(), file1->size()) == 0);
    }
}

namespace suites
{
    void registerMappedFileTests(test::TestRunner& runner)
    {
        runner.add("utils: MappedFile::open existing file", testOpenExistingFile);
        runner.add("utils: MappedFile::open non-existent file", testOpenNonExistentFile);
        runner.add("utils: MappedFile::open empty file", testOpenEmptyFile);
        runner.add("utils: MappedFile::open same file twice", testOpenSameFileTwice);
    }
}
