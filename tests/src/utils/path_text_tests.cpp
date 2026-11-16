#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/utils/file_system.hpp"

namespace
{
    // Имя, расширение и основа имени / The name, the extension and the stem
    void testNameParts()
    {
        TEST_ASSERT(gran::fs::fileName("a/b.txt") == "b.txt");
        TEST_ASSERT(gran::fs::fileName("a/") == "");
        TEST_ASSERT(gran::fs::fileExtension("a/b.tar.gz") == ".gz");
        TEST_ASSERT(gran::fs::fileExtension(".profile") == "");
        TEST_ASSERT(gran::fs::fileExtension("..") == "");
        TEST_ASSERT(gran::fs::fileStem("a/b.txt") == "b");
    }

    // Родитель: лишние разделители снимаются, корень остаётся / The parent: extra separators are dropped, the root stays
    void testParentPath()
    {
        TEST_ASSERT(gran::fs::parentPath("a/b") == "a");
        TEST_ASSERT(gran::fs::parentPath("a//b") == "a");
        TEST_ASSERT(gran::fs::parentPath("/a") == "/");
        TEST_ASSERT(gran::fs::parentPath("/") == "/");
        TEST_ASSERT(gran::fs::parentPath("a") == "");
        TEST_ASSERT(!gran::fs::isAbsolutePath("a/b"));
    }

    // Склейка и нормализация без диска / Joining and normalization without the disk
    void testJoinNormalize()
    {
        TEST_ASSERT(gran::fs::joinPath("a", "b") == "a/b");
        TEST_ASSERT(gran::fs::joinPath("a/", "b") == "a/b");
        TEST_ASSERT(gran::fs::joinPath("", "b") == "b");
        TEST_ASSERT(gran::fs::joinPath("a", "") == "a");
        TEST_ASSERT(gran::fs::normalizePath("a/./b/../c") == "a/c");
        TEST_ASSERT(gran::fs::normalizePath("a/..") == ".");
        TEST_ASSERT(gran::fs::normalizePath("../a") == "../a");
        TEST_ASSERT(gran::fs::normalizePath("") == "");
        TEST_ASSERT(gran::fs::normalizePath("a//b/") == "a/b");
    }

#if defined(_WIN32)
    // Windows: буква диска, «\», сетевой путь; «/a» без буквы — не абсолютный
    // Windows: a drive letter, "\", a network path; "/a" without a letter is not absolute
    void testPlatformPaths()
    {
        TEST_ASSERT(gran::fs::fileName("C:\\a\\b.txt") == "b.txt");
        TEST_ASSERT(gran::fs::parentPath("C:\\a\\b") == "C:\\a");
        TEST_ASSERT(gran::fs::parentPath("C:/a") == "C:/");
        TEST_ASSERT(gran::fs::parentPath("C:a") == "C:");
        TEST_ASSERT(gran::fs::isAbsolutePath("C:/a") && gran::fs::isAbsolutePath("C:\\a"));
        TEST_ASSERT(gran::fs::isAbsolutePath("//srv/share") && !gran::fs::isAbsolutePath("/a"));
        TEST_ASSERT(gran::fs::joinPath("C:/x", "/y") == "C:/y");
        TEST_ASSERT(gran::fs::joinPath("C:", "a") == "C:a");
        TEST_ASSERT(gran::fs::normalizePath("C:\\a\\..\\b") == "C:/b");
        TEST_ASSERT(gran::fs::normalizePath("C:/..") == "C:/");
        // Ревью Т-57: сетевой корень при склейке с «/…» и тройные ведущие разделители (корень без имени, как «/a»)
        // The T-57 review: the network root when joining with "/…" and triple leading separators (a root without a name, as "/a")
        TEST_ASSERT(gran::fs::joinPath("//srv/share", "/sub") == "//srv/sub");
        TEST_ASSERT(gran::fs::joinPath("\\\\srv\\share", "/sub") == "//srv/sub");
        TEST_ASSERT(gran::fs::joinPath("/x", "/y") == "/y");
        TEST_ASSERT(gran::fs::normalizePath("///a") == "/a" && !gran::fs::isAbsolutePath("///a"));
        TEST_ASSERT(gran::fs::parentPath("//srv/share") == "//srv/");
    }
#else
    // POSIX: корень «/», обратная косая — часть имени / POSIX: the root is "/", a backslash is a part of the name
    void testPlatformPaths()
    {
        TEST_ASSERT(gran::fs::isAbsolutePath("/a"));
        TEST_ASSERT(gran::fs::joinPath("a", "/b") == "/b");
        TEST_ASSERT(gran::fs::normalizePath("/../a") == "/a");
        TEST_ASSERT(gran::fs::normalizePath("///a//b") == "/a/b");
        TEST_ASSERT(gran::fs::fileName("a\\b") == "a\\b");
    }
#endif
}

namespace suites
{
    void registerPathTextTests(test::TestRunner& runner)
    {
        runner.add("utils: path name parts", testNameParts);
        runner.add("utils: path parentPath", testParentPath);
        runner.add("utils: path join and normalize", testJoinNormalize);
        runner.add("utils: path platform rules", testPlatformPaths);
    }
}
