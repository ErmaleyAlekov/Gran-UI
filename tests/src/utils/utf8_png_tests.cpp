#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/utils/utf8.hpp"
#include "gran/utils/png_writer.hpp"
#include "gran/image/image.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>

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

        std::size_t emptyPosition = 0;
        TEST_ASSERT(gran::decodeUtf8({}, emptyPosition) == gran::REPLACEMENT_CHARACTER);
        TEST_ASSERT(emptyPosition == 0);
        std::size_t exhaustedPosition = text1.size();
        TEST_ASSERT(gran::decodeUtf8(text1, exhaustedPosition) == gran::REPLACEMENT_CHARACTER);
        TEST_ASSERT(exhaustedPosition == text1.size());
        std::size_t beyondPosition = text1.size() + 1;
        TEST_ASSERT(gran::decodeUtf8(text1, beyondPosition) == gran::REPLACEMENT_CHARACTER);
        TEST_ASSERT(beyondPosition == text1.size() + 1);
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

    std::uint32_t pngWord(const std::vector<std::uint8_t>& bytes, std::size_t offset)
    {
        return (static_cast<std::uint32_t>(bytes[offset]) << 24) | (static_cast<std::uint32_t>(bytes[offset + 1]) << 16)
            | (static_cast<std::uint32_t>(bytes[offset + 2]) << 8) | bytes[offset + 3];
    }

    void testStreamingPng()
    {
        constexpr std::uint32_t width = 128;
        constexpr std::uint32_t height = 140;
        std::vector<std::uint8_t> pixels(width * height * 4, 255);
        for (std::size_t offset = 0; offset < pixels.size(); offset += 4)
            pixels[offset] = static_cast<std::uint8_t>(offset / 4);
        const auto png = gran::encodePng(pixels.data(), pixels.size(), width, height);
        std::size_t idats = 0;
        for (std::size_t position = 8; position < png.size();)
        {
            TEST_ASSERT(png.size() - position >= 12);
            const std::size_t length = pngWord(png, position);
            TEST_ASSERT(length <= png.size() - position - 12);
            TEST_ASSERT(gran::crc32(png.data() + position + 4, length + 4) == pngWord(png, position + 8 + length));
            if (pngWord(png, position + 4) == 0x49444154u)
                ++idats;
            position += 12 + length;
        }
        TEST_ASSERT(idats >= 2);
        const auto decoded = gran::image::decodePng(png.data(), png.size(), "streaming test");
        TEST_ASSERT(decoded.width == width && decoded.height == height && decoded.pixels == pixels);

        const auto path = std::filesystem::temp_directory_path() / std::filesystem::u8path(u8"gran-streaming-тест-日.png");
        const bool wrote = gran::writePng(path.u8string(), pixels.data(), pixels.size(), width, height);
        const bool rejected = !gran::writePng(path.u8string(), pixels.data(), pixels.size() - 1, width, height)
            && !gran::writePng(path.u8string(), pixels.data(), pixels.size(), 0, height);
        std::ifstream file(path, std::ios::binary);
        const std::vector<std::uint8_t> disk{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        file.close();
        std::error_code error;
        std::filesystem::remove(path, error);
        TEST_ASSERT(wrote && rejected && disk == png);
        TEST_ASSERT(gran::encodePng(nullptr, pixels.size(), width, height).empty());
        TEST_ASSERT(gran::encodePng(pixels.data(), pixels.size(), width, 0).empty());
        TEST_ASSERT(gran::encodePng(pixels.data(), pixels.size(), UINT32_MAX, height).empty());
        TEST_ASSERT(gran::encodePng(pixels.data(), pixels.size() - 1, width, height).empty());
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
        runner.add("utils: streams one PNG zlib payload through multiple IDAT chunks", testStreamingPng);
        runner.add("utils: decodeUtf8", testDecodeUtf8);
        runner.add("utils: countUtf8", testCountUtf8);
        runner.add("utils: appendUtf8", testAppendUtf8);
        runner.add("utils: crc32, adler32, encodePng", testCrc32Adler32EncodePng);
    }
}
