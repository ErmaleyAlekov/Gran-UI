#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/utils/uri_list.hpp"

namespace
{
    // Тест percentDecode
    void testPercentDecode()
    {
        std::string out;
        TEST_ASSERT(gran::percentDecode("%D0%BF%20a", out) && out == "п a");
        TEST_ASSERT(gran::percentDecode("abc%2fd", out) && out == "abc/d");
        TEST_ASSERT(!gran::percentDecode("%", out));
        TEST_ASSERT(!gran::percentDecode("%4", out));
        TEST_ASSERT(!gran::percentDecode("%zz", out));
        TEST_ASSERT(!gran::percentDecode("a%00b", out));
    }

    // Тест fileUriPath
    void testFileUriPath()
    {
        std::string path;
        TEST_ASSERT(gran::fileUriPath("file:///tmp/a.txt", path) && path == "/tmp/a.txt");
        TEST_ASSERT(gran::fileUriPath("FILE://LocalHost/x", path) && path == "/x");
        TEST_ASSERT(gran::fileUriPath("file:/y", path) && path == "/y");
        TEST_ASSERT(gran::fileUriPath("file:///a%20b?q#f", path) && path == "/a b");
        TEST_ASSERT(!gran::fileUriPath("file://server/x", path));
        TEST_ASSERT(!gran::fileUriPath("http://x/y", path));
        TEST_ASSERT(!gran::fileUriPath("trash:///x", path));
        TEST_ASSERT(!gran::fileUriPath("file:relative", path));
        TEST_ASSERT(!gran::fileUriPath("file://host", path));
    }

    // Тест parseUriList
    void testParseUriList()
    {
        std::vector<std::string> result = gran::parseUriList("# c\r\nfile:///a\r\n\r\nhttp://x/y\r\nfile:///b%20c\nfile:///d");
        TEST_ASSERT((result == std::vector<std::string>{"/a", "/b c", "/d"}));

        result = gran::parseUriList("");
        TEST_ASSERT(result.empty());
    }
}

namespace suites
{
    // Регистрация набора
    void registerUriListTests(test::TestRunner& runner)
    {
        runner.add("uri-list: percentDecode", testPercentDecode);
        runner.add("uri-list: fileUriPath", testFileUriPath);
        runner.add("uri-list: parseUriList", testParseUriList);
    }
}
