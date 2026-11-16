#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/binding/internal/extent_index.hpp"
#include <limits>

namespace
{
    void testExtentIndexEmpty()
    {
        gran::binding::ExtentIndex index;

        TEST_ASSERT(index.size() == 0);
        TEST_ASSERT(index.estimate() == 0.0f);
        TEST_ASSERT(index.offsetOf(0) == 0.0);
        TEST_ASSERT(index.indexAt(5) == 0);
        TEST_ASSERT(index.total() == 0.0);
    }

    void testExtentIndexInsertSet()
    {
        gran::binding::ExtentIndex index;

        index.insert(0, 4);
        TEST_ASSERT(index.estimate() == 0.0f);

        index.set(1, 10.0f);
        TEST_ASSERT_NEAR(index.estimate(), 10.0f, 0.001f);
        TEST_ASSERT_NEAR(index.offsetOf(4), 40.0, 0.001);
        TEST_ASSERT_NEAR(index.sizeAt(0), 10.0f, 0.001f);
    }

    void testExtentIndexMultipleSets()
    {
        gran::binding::ExtentIndex index;

        index.insert(0, 3);
        index.set(0, 10.0f);
        index.set(1, 30.0f);
        index.set(2, 20.0f);

        TEST_ASSERT_NEAR(index.offsetOf(0), 0.0, 0.001);
        TEST_ASSERT_NEAR(index.offsetOf(1), 10.0, 0.001);
        TEST_ASSERT_NEAR(index.offsetOf(2), 40.0, 0.001);
        TEST_ASSERT_NEAR(index.offsetOf(3), 60.0, 0.001);
        TEST_ASSERT_NEAR(index.total(), 60.0, 0.001);
        TEST_ASSERT(index.indexAt(-1) == 0);
        TEST_ASSERT(index.indexAt(0) == 0);
        TEST_ASSERT(index.indexAt(9.9) == 0);
        TEST_ASSERT(index.indexAt(10) == 1);
        TEST_ASSERT(index.indexAt(39) == 1);
        TEST_ASSERT(index.indexAt(40) == 2);
        TEST_ASSERT(index.indexAt(1000) == 2);
    }

    void testExtentIndexMixed()
    {
        gran::binding::ExtentIndex index;

        index.insert(0, 10);
        index.set(0, 100.0f);
        TEST_ASSERT_NEAR(index.estimate(), 100.0f, 0.001f);
        TEST_ASSERT_NEAR(index.total(), 1000.0, 0.001);

        index.set(9, 0.0f);
        TEST_ASSERT_NEAR(index.estimate(), 50.0f, 0.001f);
        TEST_ASSERT_NEAR(index.offsetOf(9), 500.0, 0.001);
    }

    void testExtentIndexForget()
    {
        gran::binding::ExtentIndex index;

        index.insert(0, 3);
        index.set(0, 10.0f);
        index.set(2, 20.0f);
        TEST_ASSERT_NEAR(index.estimate(), 15.0f, 0.001f);
        index.forget(2);
        TEST_ASSERT_NEAR(index.estimate(), 10.0f, 0.001f);
        TEST_ASSERT_NEAR(index.sizeAt(2), 10.0f, 0.001f);
    }

    void testExtentIndexInsertMiddle()
    {
        gran::binding::ExtentIndex index;

        index.insert(0, 3);
        index.set(0, 10.0f);
        index.set(1, 20.0f);
        index.set(2, 30.0f);
        index.insert(1, 2);

        TEST_ASSERT(index.size() == 5);
        TEST_ASSERT_NEAR(index.sizeAt(0), 10.0f, 0.001f);
        TEST_ASSERT_NEAR(index.sizeAt(3), 20.0f, 0.001f);
        TEST_ASSERT_NEAR(index.sizeAt(4), 30.0f, 0.001f);
        TEST_ASSERT_NEAR(index.sizeAt(1), index.estimate(), 0.001f);

        index.erase(1, 2);
        TEST_ASSERT_NEAR(index.offsetOf(3), 60.0, 0.001);

        index.erase(1, std::numeric_limits<std::size_t>::max());
        TEST_ASSERT(index.size() == 1);
    }

    void testExtentIndexPermute()
    {
        gran::binding::ExtentIndex index;

        index.insert(0, 3);
        index.set(0, 10.0f);
        index.set(1, 20.0f);
        index.set(2, 30.0f);
        index.permute({2, 0, 1});

        TEST_ASSERT_NEAR(index.sizeAt(0), 20.0f, 0.001f);
        TEST_ASSERT_NEAR(index.sizeAt(1), 30.0f, 0.001f);
        TEST_ASSERT_NEAR(index.sizeAt(2), 10.0f, 0.001f);

        index.permute({0});
        TEST_ASSERT_NEAR(index.estimate(), 0.0f, 0.001f);
    }

    void testExtentIndexSetNegativeNaN()
    {
        gran::binding::ExtentIndex index;

        index.insert(0, 3);
        index.set(2, 50.0f);
        index.set(0, -10.0f);
        TEST_ASSERT_NEAR(index.sizeAt(0), 0.0f, 0.001f);

        index.set(1, std::numeric_limits<float>::quiet_NaN());
        TEST_ASSERT_NEAR(index.sizeAt(1), 0.0f, 0.001f);
    }

    void testExtentIndexLarge()
    {
        gran::binding::ExtentIndex index;

        index.insert(0, 100000);
        for (std::size_t i = 0; i < 100000; ++i)
            index.set(i, 1 + i % 3);

        TEST_ASSERT_NEAR(index.offsetOf(100000), 199999.0, 0.001);

        for (std::size_t k : {0, 1, 777, 99999})
            TEST_ASSERT(index.indexAt(index.offsetOf(k)) == k);
    }

    // Ревью Т-60: indexAt сразу после вставки (дерево устарело) — спуск по свежему дереву / Review T-60: indexAt right after an insertion (the tree is stale) — the descent goes over a fresh tree
    void testExtentIndexIndexAtAfterInsert()
    {
        gran::binding::ExtentIndex index;

        index.insert(0, 2);
        index.set(0, 10.0f);
        index.set(1, 10.0f);
        TEST_ASSERT(index.indexAt(15.0) == 1);
        index.insert(0, 5);
        TEST_ASSERT(index.indexAt(65.0) == 6);
        index.erase(0, 6);
        TEST_ASSERT(index.indexAt(5.0) == 0);
        index.permute({0});
        TEST_ASSERT(index.indexAt(5.0) == 0);
    }
}

namespace suites
{
    void registerExtentIndexTests(test::TestRunner& runner)
    {
        runner.add("extent index: empty", testExtentIndexEmpty);
        runner.add("extent index: index at after insert", testExtentIndexIndexAtAfterInsert);
        runner.add("extent index: insert set", testExtentIndexInsertSet);
        runner.add("extent index: multiple sets", testExtentIndexMultipleSets);
        runner.add("extent index: mixed", testExtentIndexMixed);
        runner.add("extent index: forget", testExtentIndexForget);
        runner.add("extent index: insert middle", testExtentIndexInsertMiddle);
        runner.add("extent index: permute", testExtentIndexPermute);
        runner.add("extent index: set negative NaN", testExtentIndexSetNegativeNaN);
        runner.add("extent index: large", testExtentIndexLarge);
    }
}
