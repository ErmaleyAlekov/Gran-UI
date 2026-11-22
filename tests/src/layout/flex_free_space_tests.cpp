#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/layout/internal/layout_internal.hpp"

namespace
{
    // После ограничения первого элемента проверяем оба оставшихся.
    // After clamping the first item, check both remaining items.
    void testFractionalGrowthAfterFreeze()
    {
        gran::ui::Node nodes[3];
        std::vector<gran::layout::internal::FlexItem> items(3);

        nodes[0].computed.grow = 0.8f;
        nodes[1].computed.grow = nodes[2].computed.grow = 0.25f;
        for (std::size_t index = 0; index < items.size(); ++index)
        {
            items[index].node = &nodes[index];
            items[index].base = 40.0f;
            items[index].maxMain = index == 0 ? 42.0f : 100.0f;
        }
        gran::layout::internal::resolveFlexibleLengths(items, 0, items.size(), 140.0f);
        TEST_ASSERT_NEAR(items[0].target, 42.0f, 0.01f);
        TEST_ASSERT_NEAR(items[1].target, 45.0f, 0.01f);
        TEST_ASSERT_NEAR(items[2].target, 45.0f, 0.01f);
    }

    void testZeroBasisParticipatesInShrinkFactorSum()
    {
        gran::ui::Node nodes[2];
        std::vector<gran::layout::internal::FlexItem> items(2);

        nodes[0].computed.shrink = nodes[1].computed.shrink = 0.5f;
        items[0].node = &nodes[0];
        items[1].node = &nodes[1];
        items[0].base = 100.0f;
        items[0].maxMain = items[1].maxMain = 100.0f;
        gran::layout::internal::resolveFlexibleLengths(items, 0, items.size(), 50.0f);
        TEST_ASSERT_NEAR(items[0].target, 50.0f, 0.01f);
        TEST_ASSERT_NEAR(items[1].target, 0.0f, 0.01f);
    }
}

namespace suites
{
    void registerFlexFreeSpaceTests(test::TestRunner& runner)
    {
        runner.add("flex: fractional growth after freezing an item", testFractionalGrowthAfterFreeze);
        runner.add("flex: zero basis participates in shrink factor sum", testZeroBasisParticipatesInShrinkFactorSum);
    }
}
