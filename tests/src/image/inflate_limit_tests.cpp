#include "framework/test_framework.hpp"
#include "gran/image/internal/inflate_internal.hpp"

namespace
{
    void testInflateLimitsBuildHuffman()
    {
        gran::image::internal::Huffman code;
        std::uint16_t lengths1[] = {1, 2, 16};
        TEST_ASSERT(!gran::image::internal::buildHuffman(code, lengths1, 3));

        std::uint16_t lengths2[] = {1, 2, 15};
        TEST_ASSERT(gran::image::internal::buildHuffman(code, lengths2, 3));

        std::uint16_t lengths3[] = {1, 1, 1};
        TEST_ASSERT(!gran::image::internal::buildHuffman(code, lengths3, 3));
    }

    void testInflateLimitsReadBits()
    {
        std::uint8_t data1[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        gran::image::internal::BitReader reader1{data1, 8};
        TEST_ASSERT(gran::image::internal::readBits(reader1, gran::image::internal::MAX_READ_BITS) == 0xFFFFFF);
        TEST_ASSERT(!reader1.failed);

        gran::image::internal::BitReader reader2{data1, 8};
        TEST_ASSERT(gran::image::internal::readBits(reader2, gran::image::internal::MAX_READ_BITS + 1) == 0);
        TEST_ASSERT(reader2.failed);

        gran::image::internal::BitReader reader3{data1, 8};
        TEST_ASSERT(gran::image::internal::readBits(reader3, 32) == 0);
        TEST_ASSERT(reader3.failed);

        gran::image::internal::BitReader reader4{data1, 8};
        TEST_ASSERT(gran::image::internal::readBits(reader4, -1) == 0);
        TEST_ASSERT(reader4.failed);
    }

    void testInflateLimitsBitReaderSingleByte()
    {
        std::uint8_t data[] = {0xFF};
        gran::image::internal::BitReader reader{data, 1};
        TEST_ASSERT(gran::image::internal::readBits(reader, 9) == 0);
        TEST_ASSERT(reader.failed);
    }
}

namespace suites
{
    void registerInflateLimitTests(test::TestRunner& runner)
    {
        runner.add("inflate limits: build huffman", testInflateLimitsBuildHuffman);
        runner.add("inflate limits: read bits", testInflateLimitsReadBits);
        runner.add("inflate limits: bit reader single byte", testInflateLimitsBitReaderSingleByte);
    }
}
