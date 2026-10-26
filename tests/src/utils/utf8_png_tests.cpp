#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/utils/utf8.hpp"
#include "gran/utils/png_writer.hpp"

namespace
{
    void testDecodeUtf8()
    {
        std::string_view text1 = "\xD0\x96";
        std::size_t pos1 = 0;
        TEST_ASSERT(gran::decodeUtf8(text1, pos1) == 0x416);
        TEST_ASSERT(pos1 == 2);

        pos1 = 0;
        TEST_ASSERT(gran::decodeUtf8(text1, pos1) == 0x416);
        TEST_ASSERT(pos1 == 2);

        std::string_view text2 = "\xFF";
        std::size_t pos2 = 0;
        TEST_ASSERT(gran::decodeUtf8(text2, pos2) == gran::REPLACEMENT_CHARACTER);
        TEST_ASSERT(pos2 == 1);

        std::string_view text3 = "\xC0\x80";
        std::size_t pos3 = 0;
        TEST_ASSERT(gran::decodeUtf8(text3, pos3) == gran::REPLACEMENT_CHARACTER);
        TEST_ASSERT(pos3 == 1);

        std::string_view text4 = "\xED\xA0\x80";
        std::size_t pos4 = 0;
        TEST_ASSERT(gran::decodeUtf8(text4, pos4) == gran::REPLACEMENT_CHARACTER);
        TEST_ASSERT(pos4 == 1);

        std::string_view text5 = "\xF0\x9F\x98\x80";
        std::size_t pos5 = 0;
        TEST_ASSERT(gran::decodeUtf8(text5, pos5) == 0x1F600);
        TEST_ASSERT(pos5 == 4);
    }

    void testCountUtf8()
    {
        std::string_view text = "a\xD0\x96\xF0\x9F\x98\x80";
        TEST_ASSERT(gran::countUtf8(text) == 3);
    }

    void testAppendUtf8()
    {
        std::string text;
        gran::appendUtf8(text, 0x1F600);
        TEST_ASSERT(text.size() == 4);
        std::size_t pos = 0;
        TEST_ASSERT(gran::decodeUtf8(text, pos) == 0x1F600);
    }

    void testCrc32Adler32EncodePng()
    {
        std::uint8_t data[] = {255, 0, 0, 255};
        auto png = gran::encodePng(data, 1, 1);
        TEST_ASSERT(png.size() > 0);

        TEST_ASSERT(png[0] == 0x89);
        TEST_ASSERT(png[1] == 'P');
        TEST_ASSERT(png[2] == 'N');
        TEST_ASSERT(png[3] == 'G');
        TEST_ASSERT(png[4] == 0x0D);
        TEST_ASSERT(png[5] == 0x0A);
        TEST_ASSERT(png[6] == 0x1A);
        TEST_ASSERT(png[7] == 0x0A);

        TEST_ASSERT(png[12] == 'I');
        TEST_ASSERT(png[13] == 'H');
        TEST_ASSERT(png[14] == 'D');
        TEST_ASSERT(png[15] == 'R');

        TEST_ASSERT(gran::crc32(reinterpret_cast<const std::uint8_t*>("IEND"), 4) == 0xAE426082u);
        TEST_ASSERT(gran::adler32(reinterpret_cast<const std::uint8_t*>("Wikipedia"), 9) == 0x11E60398u);
    }
}

namespace suites
{
    void registerUtf8PngTests(test::TestRunner& runner)
    {
        runner.add("utils: decodeUtf8", testDecodeUtf8);
        runner.add("utils: countUtf8", testCountUtf8);
        runner.add("utils: appendUtf8", testAppendUtf8);
        runner.add("utils: crc32, adler32, encodePng", testCrc32Adler32EncodePng);
    }
}
