#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/rhi/adapter_choice.hpp"

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

    void registerAdapterChoiceTests(test::TestRunner& runner)
    {
        runner.add("adapter_matches", testAdapterMatches);
    }
}
