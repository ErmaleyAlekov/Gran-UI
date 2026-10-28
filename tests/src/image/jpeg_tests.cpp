#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/image/image.hpp"
#include "support/jpeg_samples.hpp"

#include <cstdlib>

namespace
{
    void checkSample(const std::uint8_t* data, std::size_t size, const std::uint8_t* rgb)
    {
        gran::image::Image image = gran::image::decodeJpeg(data, size, "test");
        TEST_ASSERT(image.width == jpeg_samples::WIDTH);
        TEST_ASSERT(image.height == jpeg_samples::HEIGHT);
        TEST_ASSERT(image.pixels.size() == jpeg_samples::WIDTH * jpeg_samples::HEIGHT * 4);

        for (std::size_t i = 0; i < image.pixels.size() / 4; ++i)
        {
            for (std::size_t c = 0; c < 3; ++c)
            {
                TEST_ASSERT(std::abs(static_cast<int>(image.pixels[i * 4 + c]) - rgb[i * 3 + c]) <= 3);
            }
            TEST_ASSERT(image.pixels[i * 4 + 3] == 255);
        }
    }

    void testBaseline420()
    {
        checkSample(jpeg_samples::BASELINE_420, jpeg_samples::BASELINE_420_SIZE, jpeg_samples::BASELINE_420_RGB);
    }

    void testProgressive420()
    {
        checkSample(jpeg_samples::PROGRESSIVE_420, jpeg_samples::PROGRESSIVE_420_SIZE, jpeg_samples::PROGRESSIVE_420_RGB);
    }

    void testBaseline444Restart()
    {
        checkSample(jpeg_samples::BASELINE_444_RESTART, jpeg_samples::BASELINE_444_RESTART_SIZE, jpeg_samples::BASELINE_444_RESTART_RGB);
    }

    void testProgressiveGray()
    {
        checkSample(jpeg_samples::PROGRESSIVE_GRAY, jpeg_samples::PROGRESSIVE_GRAY_SIZE, jpeg_samples::PROGRESSIVE_GRAY_RGB);
    }

    void testDecodeImageBaseline420()
    {
        gran::image::Image imageJpeg = gran::image::decodeJpeg(jpeg_samples::BASELINE_420, jpeg_samples::BASELINE_420_SIZE, "test");
        gran::image::Image imageAny = gran::image::decodeImage(jpeg_samples::BASELINE_420, jpeg_samples::BASELINE_420_SIZE, "test");
        TEST_ASSERT(imageJpeg.pixels == imageAny.pixels);
    }

    void testExceptions()
    {
        bool thrown = false;
        try
        {
            gran::image::decodeJpeg(nullptr, 0, "test");
        }
        catch (const gran::GranException&)
        {
            thrown = true;
        }
        TEST_ASSERT(thrown);

        thrown = false;
        try
        {
            gran::image::decodeJpeg(reinterpret_cast<const std::uint8_t*>( "\x12\x34\x56\x78"), 4, "test");
        }
        catch (const gran::GranException&)
        {
            thrown = true;
        }
        TEST_ASSERT(thrown);

        thrown = false;
        try
        {
            gran::image::decodeJpeg(jpeg_samples::BASELINE_420, 20, "test");
        }
        catch (const gran::GranException&)
        {
            thrown = true;
        }
        TEST_ASSERT(thrown);

        thrown = false;
        try
        {
            gran::image::decodeImage(reinterpret_cast<const std::uint8_t*>( "\x01\x02\x03\x04\x05\x06\x07\x08"), 8, "test");
        }
        catch (const gran::GranException&)
        {
            thrown = true;
        }
        TEST_ASSERT(thrown);
    }
}

namespace suites
{
    void registerJpegTests(test::TestRunner& runner)
    {
        runner.add("image: decodeJpeg baseline 420", testBaseline420);
        runner.add("image: decodeJpeg progressive 420", testProgressive420);
        runner.add("image: decodeJpeg baseline 444 restart", testBaseline444Restart);
        runner.add("image: decodeJpeg progressive gray", testProgressiveGray);
        runner.add("image: decodeImage baseline 420", testDecodeImageBaseline420);
        runner.add("image: exceptions", testExceptions);
    }
}
