#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/render/pixel_compare.hpp"
#include "gran/image/image.hpp"

namespace suites
{
    void testEquality()
    {
        gran::image::Image img1 = {1, 1, {10, 20, 30, 255}};
        gran::image::Image img2 = {1, 1, {10, 20, 30, 255}};
        gran::render::PixelComparison result = gran::render::comparePixels(img1, img2, 0);
        TEST_ASSERT(result.sameSize == true);
        TEST_ASSERT(result.differentPixels == 0);
        TEST_ASSERT(result.maxDelta == 0);

        img2.pixels[1] = 21;
        result = gran::render::comparePixels(img1, img2, 1);
        TEST_ASSERT(result.sameSize == true);
        TEST_ASSERT(result.differentPixels == 0);
        TEST_ASSERT(result.maxDelta == 1);

        result = gran::render::comparePixels(img1, img2, 0);
        TEST_ASSERT(result.sameSize == true);
        TEST_ASSERT(result.differentPixels == 1);
        TEST_ASSERT(result.maxDelta == 1);
    }

    void testAlpha()
    {
        gran::image::Image img1 = {1, 1, {10, 20, 30, 255}};
        gran::image::Image img2 = {1, 1, {10, 20, 30, 250}};
        gran::render::PixelComparison result = gran::render::comparePixels(img1, img2, 2);
        TEST_ASSERT(result.sameSize == true);
        TEST_ASSERT(result.differentPixels == 1);
        TEST_ASSERT(result.maxDelta == 5);
    }

    void testSize()
    {
        gran::image::Image img1 = {1, 1, {10, 20, 30, 255}};
        gran::image::Image img2 = {1, 1, {10, 20, 30, 255}};
        img2.pixels.pop_back();
        gran::render::PixelComparison result = gran::render::comparePixels(img1, img2, 0);
        TEST_ASSERT(result.sameSize == false);
        img2 = {2, 1, {10, 20, 30, 255, 10, 20, 30, 255}};
        TEST_ASSERT(!gran::render::comparePixels(img1, img2, 0).sameSize);
    }

    void registerPixelCompareTests(test::TestRunner& runner)
    {
        runner.add("equality", testEquality);
        runner.add("alpha", testAlpha);
        runner.add("size", testSize);
    }
}
