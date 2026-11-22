#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/rhi/adapter_choice.hpp"

#ifdef GRAN_WITH_VULKAN
#include "gran/rhi/vulkan/internal/swap_extent.hpp"
#endif

namespace suites
{
    void testAdapterMatches()
    {
        TEST_ASSERT(gran::rhi::adapterMatches("", "Intel(R) UHD Graphics 630"));
        TEST_ASSERT(gran::rhi::adapterMatches("", ""));
        TEST_ASSERT(gran::rhi::adapterMatches("intel", "Intel(R) UHD Graphics 630"));
        TEST_ASSERT(gran::rhi::adapterMatches("NVIDIA", "nvidia geforce rtx 3060"));
        TEST_ASSERT(gran::rhi::adapterMatches("rtx 3060", "nvidia geforce rtx 3060"));
        TEST_ASSERT(!gran::rhi::adapterMatches("amd", "Intel(R) UHD Graphics 630"));
        TEST_ASSERT(!gran::rhi::adapterMatches("Intel(R) UHD Graphics 630", "intel"));
        TEST_ASSERT(gran::rhi::adapterMatches("630", "Intel(R) UHD Graphics 630"));
    }

#ifdef GRAN_WITH_VULKAN
    // Каждая нулевая ось WSI откладывает создание цепочки даже при ненулевом запросе.
    // Either zero WSI axis defers swapchain creation even with a nonzero request.
    void testVulkanSwapExtent()
    {
        VkSurfaceCapabilitiesKHR caps{};
        VkExtent2D extent{};
        const VkExtent2D requested{640, 480};

        caps.currentExtent = {0, 480};
        TEST_ASSERT(!gran::rhi::vk::chooseSwapExtent(caps, requested, extent));
        caps.currentExtent = {640, 0};
        TEST_ASSERT(!gran::rhi::vk::chooseSwapExtent(caps, requested, extent));
        caps.currentExtent = {800, 600};
        TEST_ASSERT(gran::rhi::vk::chooseSwapExtent(caps, requested, extent));
        TEST_ASSERT(extent.width == 800 && extent.height == 600);
        caps.currentExtent = {UINT32_MAX, UINT32_MAX};
        caps.minImageExtent = {700, 100};
        caps.maxImageExtent = {1000, 400};
        TEST_ASSERT(gran::rhi::vk::chooseSwapExtent(caps, requested, extent));
        TEST_ASSERT(extent.width == 700 && extent.height == 400);
    }
#endif

    void registerAdapterChoiceTests(test::TestRunner& runner)
    {
        runner.add("adapter_matches", testAdapterMatches);
#ifdef GRAN_WITH_VULKAN
        runner.add("vulkan: swapchain extent", testVulkanSwapExtent);
#endif
    }
}
