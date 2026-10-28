#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/timer_queue.hpp"
#include <vector>

namespace
{
    struct TestFunctor
    {
        std::vector<int>* results;
        int value;

        TestFunctor(std::vector<int>* sink, int number) : results(sink), value(number) {}

        void operator()() const
        {
            results->push_back(value);
        }
    };

    // Ставит в ту же очередь таймер со сроком 0, который пишет 2 / Sets a zero-due timer writing 2 into the same queue
    struct SelfAddingFunctor
    {
        gran::app::TimerQueue* queue;
        std::vector<int>* results;

        SelfAddingFunctor(gran::app::TimerQueue* owner, std::vector<int>* sink) : queue(owner), results(sink) {}

        void operator()() const
        {
            results->push_back(1);
            queue->add(0.0, 0.0, TestFunctor(results, 2));
        }
    };

    struct SelfRemovingFunctor
    {
        gran::app::TimerId* id;
        gran::app::TimerQueue* queue;

        SelfRemovingFunctor(gran::app::TimerId* timer, gran::app::TimerQueue* owner) : id(timer), queue(owner) {}

        void operator()() const
        {
            queue->cancel(*id);
        }
    };

    // Счётчик живёт в самом функторе; наружу пишется только его значение / The counter lives in the functor itself; only its value is written outside
    struct CounterFunctor
    {
        int* output;
        int count = 0;

        CounterFunctor(int* sink) : output(sink) {}

        void operator()()
        {
            ++count;
            *output = count;
        }
    };

    void testAddAndSize()
    {
        gran::app::TimerQueue queue;
        gran::app::TimerId first = queue.add(1.0, 0.0, TestFunctor(nullptr, 1));
        gran::app::TimerId second = queue.add(2.0, 0.0, TestFunctor(nullptr, 2));
        TEST_ASSERT(first != 0 && second != 0 && first != second);
        TEST_ASSERT(queue.size() == 2);
    }

    void testNextDue()
    {
        gran::app::TimerQueue queue;
        TEST_ASSERT(queue.nextDue() < 0);
        queue.add(1.0, 0.0, TestFunctor(nullptr, 1));
        TEST_ASSERT(queue.nextDue() == 1.0);
        queue.add(0.5, 0.0, TestFunctor(nullptr, 2));
        TEST_ASSERT(queue.nextDue() == 0.5);
    }

    void testRunDue()
    {
        gran::app::TimerQueue queue;
        std::vector<int> results;
        queue.add(1.0, 0.0, TestFunctor(&results, 1));
        queue.add(2.0, 0.0, TestFunctor(&results, 2));
        queue.add(0.5, 0.0, TestFunctor(&results, 3));
        TEST_ASSERT(!queue.runDue(0.4));
        TEST_ASSERT(queue.runDue(1.5));
        std::vector<int> expected1{3, 1};
        TEST_ASSERT(results == expected1);
        TEST_ASSERT(queue.size() == 1);
        TEST_ASSERT(queue.runDue(3.0));
        TEST_ASSERT(queue.size() == 0);
        TEST_ASSERT(!queue.runDue(4.0));
    }

    void testPeriodicAdd()
    {
        gran::app::TimerQueue queue;
        std::vector<int> results;
        queue.add(1.0, 0.5, TestFunctor(&results, 1));
        TEST_ASSERT(queue.runDue(1.0));
        std::vector<int> expected2{1};
        TEST_ASSERT(results == expected2);
        TEST_ASSERT(queue.nextDue() == 1.5);
        TEST_ASSERT(queue.runDue(10.0));
        std::vector<int> expected3{1, 1};
        TEST_ASSERT(results == expected3);
        TEST_ASSERT(queue.nextDue() == 10.5);
    }

    void testCancel()
    {
        gran::app::TimerQueue queue;
        std::vector<int> results;
        auto id = queue.add(1.0, 0.0, TestFunctor(&results, 1));
        TEST_ASSERT(queue.cancel(id));
        TEST_ASSERT(!queue.cancel(id));
        TEST_ASSERT(!queue.cancel(999));
        TEST_ASSERT(!queue.runDue(5.0));
        TEST_ASSERT(results.empty());
    }

    void testSelfAddingFunctor()
    {
        gran::app::TimerQueue queue;
        std::vector<int> results;
        queue.add(0.0, 0.0, SelfAddingFunctor(&queue, &results));
        TEST_ASSERT(queue.runDue(0.0));
        std::vector<int> first{1};
        TEST_ASSERT(results == first);
        TEST_ASSERT(queue.runDue(0.0));
        std::vector<int> second{1, 2};
        TEST_ASSERT(results == second);
    }

    void testSelfRemovingFunctor()
    {
        gran::app::TimerQueue queue;
        gran::app::TimerId id = 0;
        id = queue.add(1.0, 0.5, SelfRemovingFunctor(&id, &queue));
        TEST_ASSERT(queue.runDue(1.0));
        TEST_ASSERT(queue.size() == 0);
        TEST_ASSERT(!queue.runDue(5.0));
    }

    void testStatePreservation()
    {
        gran::app::TimerQueue queue;
        int counter = 0;
        queue.add(1.0, 0.5, CounterFunctor(&counter));
        TEST_ASSERT(queue.runDue(1.0));
        TEST_ASSERT(queue.runDue(1.5));
        TEST_ASSERT(queue.runDue(2.0));
        TEST_ASSERT(counter == 3);
    }
}

namespace suites
{
    void registerTimerQueueTests(test::TestRunner& runner)
    {
        runner.add("timer_queue: add and size", testAddAndSize);
        runner.add("timer_queue: nextDue", testNextDue);
        runner.add("timer_queue: runDue", testRunDue);
        runner.add("timer_queue: periodic add", testPeriodicAdd);
        runner.add("timer_queue: cancel", testCancel);
        runner.add("timer_queue: self adding functor", testSelfAddingFunctor);
        runner.add("timer_queue: self removing functor", testSelfRemovingFunctor);
        runner.add("timer_queue: state preservation", testStatePreservation);
    }
}
