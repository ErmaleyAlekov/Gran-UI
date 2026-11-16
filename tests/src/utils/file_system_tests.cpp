#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/utils/file_system.hpp"
#include "gran/utils/mapped_file.hpp"
#include <fstream>
#include <string>
#include <vector>

namespace
{
    void clearTestDirectory(const std::string& root)
    {
        for (const auto& path : gran::fs::listFiles(root, true))
        {
            gran::fs::removePath(gran::fs::joinPath(root, path));
        }
        gran::fs::removePath(gran::fs::joinPath(root, "a/b"));
        gran::fs::removePath(gran::fs::joinPath(root, "a"));
        gran::fs::removePath(root);
    }

    void testCreateDirectories()
    {
        std::string root = gran::fs::joinPath(gran::fs::temporaryDirectory(), "gran_fs_\xD1\x82\xD0\xB5\xD1\x81\xD1\x82");
        clearTestDirectory(root);

        TEST_ASSERT(gran::fs::createDirectories(gran::fs::joinPath(root, "a/b")));
        TEST_ASSERT(gran::fs::isDirectory(gran::fs::joinPath(root, "a/b")));
        TEST_ASSERT(gran::fs::pathExists(gran::fs::joinPath(root, "a/b")));

        TEST_ASSERT(gran::fs::createDirectories(gran::fs::joinPath(root, "a/b")));
        TEST_ASSERT(gran::fs::isDirectory(gran::fs::joinPath(root, "a/b")));
        TEST_ASSERT(gran::fs::pathExists(gran::fs::joinPath(root, "a/b")));

        clearTestDirectory(root);
    }

    void testFileOperations()
    {
        std::string root = gran::fs::joinPath(gran::fs::temporaryDirectory(), "gran_fs_\xD1\x82\xD0\xB5\xD1\x81\xD1\x82");
        clearTestDirectory(root);
        TEST_ASSERT(gran::fs::createDirectories(root));

        std::ofstream out;
        gran::fs::openFile(out, gran::fs::joinPath(root, "\xD1\x84\xE6\x97\xA5.txt"));
        out << "hello";
        out.close();

        gran::fs::FileInfo info = gran::fs::queryPath(gran::fs::joinPath(root, "\xD1\x84\xE6\x97\xA5.txt"));
        TEST_ASSERT(info.exists);
        TEST_ASSERT(info.regular);
        TEST_ASSERT(!info.directory);
        TEST_ASSERT(info.size == 5);
        TEST_ASSERT(info.modified > 0);

        std::ifstream in;
        gran::fs::openFile(in, gran::fs::joinPath(root, "\xD1\x84\xE6\x97\xA5.txt"));
        std::string content;
        std::getline(in, content);
        TEST_ASSERT(content == "hello");
        in.close();
        // Регрессия: MappedFile (шрифты) открывал путь через CreateFileA — символ вне кодовой страницы ANSI («ф日») терялся
        // Regression: MappedFile (fonts) opened the path through CreateFileA — a symbol outside the ANSI code page ("ф日") was lost
        std::shared_ptr<const gran::MappedFile> mapped = gran::MappedFile::open(gran::fs::joinPath(root, "\xD1\x84\xE6\x97\xA5.txt"));
        TEST_ASSERT(mapped != nullptr && mapped->size() == 5);
        mapped.reset();

        clearTestDirectory(root);
    }

    void testListFiles()
    {
        std::string root = gran::fs::joinPath(gran::fs::temporaryDirectory(), "gran_fs_\xD1\x82\xD0\xB5\xD1\x81\xD1\x82");
        clearTestDirectory(root);

        gran::fs::createDirectories(gran::fs::joinPath(root, "a/b"));
        std::ofstream out1;
        gran::fs::openFile(out1, gran::fs::joinPath(root, "x.txt"));
        std::ofstream out2;
        gran::fs::openFile(out2, gran::fs::joinPath(root, "a/y.txt"));
        std::ofstream out3;
        gran::fs::openFile(out3, gran::fs::joinPath(root, "a/b/z.txt"));
        out1.close();
        out2.close();
        out3.close();

        std::vector<std::string> expected = {"a/b/z.txt", "a/y.txt", "x.txt"};
        std::vector<std::string> files = gran::fs::listFiles(root, true);
        TEST_ASSERT(files == expected);

        std::vector<std::string> top = {"x.txt"};
        std::vector<std::string> nonRecursiveFiles = gran::fs::listFiles(root, false);
        TEST_ASSERT(nonRecursiveFiles == top);

        // В корне — папка «a» и файл x.txt / The root has the "a" folder and the x.txt file
        bool folderSeen = false;
        std::vector<gran::fs::DirectoryEntry> directory = gran::fs::listDirectory(root);
        for (const gran::fs::DirectoryEntry& entry : directory)
            folderSeen = folderSeen || (entry.name == "a" && entry.info.directory);
        TEST_ASSERT(directory.size() == 2);
        TEST_ASSERT(folderSeen);

        clearTestDirectory(root);
    }

    void testRenameRemovePath()
    {
        std::string root = gran::fs::joinPath(gran::fs::temporaryDirectory(), "gran_fs_\xD1\x82\xD0\xB5\xD1\x81\xD1\x82");
        clearTestDirectory(root);

        TEST_ASSERT(gran::fs::createDirectories(root));
        std::ofstream out;
        gran::fs::openFile(out, gran::fs::joinPath(root, "source.txt"));
        out << "hello";
        out.close();
        // Цель уже есть — renamePath должен заменить её / The target exists already — renamePath must replace it
        gran::fs::openFile(out, gran::fs::joinPath(root, "target.txt"));
        out << "old";
        out.close();

        TEST_ASSERT(gran::fs::renamePath(gran::fs::joinPath(root, "source.txt"), gran::fs::joinPath(root, "target.txt")));
        TEST_ASSERT(gran::fs::queryPath(gran::fs::joinPath(root, "target.txt")).size == 5);
        TEST_ASSERT(!gran::fs::pathExists(gran::fs::joinPath(root, "source.txt")));

        TEST_ASSERT(gran::fs::removePath(gran::fs::joinPath(root, "target.txt")));
        TEST_ASSERT(!gran::fs::pathExists(gran::fs::joinPath(root, "target.txt")));

        TEST_ASSERT(!gran::fs::removePath(gran::fs::joinPath(root, "nonexistent.txt")));

        clearTestDirectory(root);
    }

    void testCurrentDirectory()
    {
        std::string current = gran::fs::currentDirectory();
        TEST_ASSERT(!current.empty());
        TEST_ASSERT(gran::fs::isAbsolutePath(current));

        TEST_ASSERT(gran::fs::absolutePath("x") == gran::fs::normalizePath(gran::fs::joinPath(current, "x")));
    }
}

namespace suites
{
    void registerFileSystemTests(test::TestRunner& runner)
    {
        runner.add("utils: fs create directories", testCreateDirectories);
        runner.add("utils: fs file operations", testFileOperations);
        runner.add("utils: fs list files", testListFiles);
        runner.add("utils: fs rename remove path", testRenameRemovePath);
        runner.add("utils: fs current directory", testCurrentDirectory);
    }
}
