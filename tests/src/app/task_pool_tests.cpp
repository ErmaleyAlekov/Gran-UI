#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/internal/task_pool.hpp"

#include <any>
#include <atomic>
#include <chrono>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace
{
    // Очередь «потока интерфейса» теста: пул кладёт сюда итоги и прогресс / The test "UI thread" queue: the pool puts the outcomes and the progress here
    struct Inbox
    {
        std::mutex mutex;
        std::vector<gran::app::UiTask> tasks;
    };

    // Отправка в очередь теста из любого потока / Sending into the test queue from any thread
    struct Poster
    {
        Inbox* inbox;

        void operator()(gran::app::UiTask task) const
        {
            std::lock_guard<std::mutex> lock(inbox->mutex);
            inbox->tasks.push_back(std::move(task));
        }
    };

    // Итоги и прогресс задач в порядке доставки / The task outcomes and progress in the delivery order
    struct Journal
    {
        std::vector<gran::TaskOutcome> outcomes;
        std::vector<double> steps;
    };

    // Выполнять доставки, пока не придёт count итогов (не дольше 2 с) / Run the deliveries until count outcomes come (at most 2 s)
    void drain(Inbox& inbox, const Journal& journal, std::size_t count)
    {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);

        while (journal.outcomes.size() < count && std::chrono::steady_clock::now() < deadline)
        {
            std::vector<gran::app::UiTask> batch;
            {
                std::lock_guard<std::mutex> lock(inbox.mutex);
                batch.swap(inbox.tasks);
            }
            for (gran::app::UiTask& task : batch)
                task();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    // Обработчики итога и прогресса пишут в журнал; прогресс после итога — ошибка (-1 в шагах) / The outcome and progress handlers write into the journal; a progress after the outcome is an error (-1 in the steps)
    struct Record
    {
        Journal* journal;

        void operator()(gran::TaskOutcome& outcome) const
        {
            journal->outcomes.push_back(outcome);
        }

        void operator()(const gran::TaskProgressInfo& info) const
        {
            if (!journal->outcomes.empty())
                journal->steps.push_back(-1.0);
            journal->steps.push_back(info.fraction);
        }
    };

    // Работа по виду: 0 — результат 42, 1 — исключение, 2 — fail, 3 — ждать отмены, 4 — ждать release, 5 — 100 шагов прогресса
    // A work by the kind: 0 — the result 42, 1 — an exception, 2 — fail, 3 — wait for cancellation, 4 — wait for release, 5 — 100 progress steps
    struct Work
    {
        int kind;
        std::atomic<bool>* flag;

        void operator()(gran::TaskContext& task) const
        {
            if (kind == 1)
                throw std::runtime_error("boom");
            if (kind == 2)
                task.fail("bad");
            while (kind == 3 && !task.cancelled())
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            while (kind == 4 && !flag->load())
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            for (int step = 1; kind == 5 && step <= 100; ++step)
                task.progress(step / 100.0, "step");
            if (flag != nullptr && kind != 4)
                flag->store(true);
            task.setResult(42);
        }
    };

    void testResultAndFailures()
    {
        Inbox inbox;
        Journal journal;
        gran::app::TaskPool pool(Poster{&inbox}, 2);

        gran::TaskId first = pool.submit(Work{0, nullptr}, Record{&journal}, gran::TaskProgress());
        pool.submit(Work{1, nullptr}, Record{&journal}, gran::TaskProgress());
        pool.submit(Work{2, nullptr}, Record{&journal}, gran::TaskProgress());
        drain(inbox, journal, 3);
        TEST_ASSERT(journal.outcomes.size() == 3);
        TEST_ASSERT(pool.active() == 0);
        for (gran::TaskOutcome& outcome : journal.outcomes)
        {
            if (outcome.id == first)
                TEST_ASSERT(outcome.status == gran::TaskStatus::DONE && std::any_cast<int>(outcome.result) == 42);
            else
                TEST_ASSERT(outcome.status == gran::TaskStatus::FAILED && !outcome.result.has_value());
            if (outcome.status == gran::TaskStatus::FAILED)
                TEST_ASSERT(outcome.error == "boom" || outcome.error == "bad");
        }
    }

    void testCancelQueuedAndRunning()
    {
        Inbox inbox;
        Journal journal;
        std::atomic<bool> release{false};
        std::atomic<bool> queuedRan{false};
        gran::app::TaskPool pool(Poster{&inbox}, 1);

        gran::TaskId blocking = pool.submit(Work{4, &release}, Record{&journal}, gran::TaskProgress());
        gran::TaskId queued = pool.submit(Work{0, &queuedRan}, Record{&journal}, gran::TaskProgress());
        TEST_ASSERT(pool.cancel(queued));
        TEST_ASSERT(!pool.cancel(12345));
        release.store(true);
        drain(inbox, journal, 2);
        TEST_ASSERT(journal.outcomes.size() == 2 && !queuedRan.load());
        TEST_ASSERT(journal.outcomes[0].id == blocking && journal.outcomes[0].status == gran::TaskStatus::DONE);
        TEST_ASSERT(journal.outcomes[1].status == gran::TaskStatus::CANCELLED);
        gran::TaskId running = pool.submit(Work{3, nullptr}, Record{&journal}, gran::TaskProgress());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        TEST_ASSERT(pool.cancel(running));
        drain(inbox, journal, 3);
        TEST_ASSERT(journal.outcomes.size() == 3 && journal.outcomes[2].status == gran::TaskStatus::CANCELLED);
        TEST_ASSERT(!journal.outcomes[2].result.has_value());
        TEST_ASSERT(!pool.cancel(running));
    }

    void testProgressMerged()
    {
        Inbox inbox;
        Journal journal;
        gran::app::TaskPool pool(Poster{&inbox}, 1);

        pool.submit(Work{5, nullptr}, Record{&journal}, Record{&journal});
        drain(inbox, journal, 1);
        TEST_ASSERT(!journal.steps.empty() && journal.steps.size() <= 100);
        TEST_ASSERT(journal.steps.back() == 1.0);
        for (double step : journal.steps)
            TEST_ASSERT(step > 0.0 && step <= 1.0);
    }

    void testDestructorCancels()
    {
        Inbox inbox;
        Journal journal;
        std::atomic<bool> finished{false};

        {
            gran::app::TaskPool pool(Poster{&inbox}, 1);
            pool.submit(Work{3, &finished}, Record{&journal}, gran::TaskProgress());
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        TEST_ASSERT(finished.load());
        TEST_ASSERT(journal.outcomes.empty());
    }
}

namespace suites
{
    void registerTaskPoolTests(test::TestRunner& runner)
    {
        runner.add("task pool: result and failures", testResultAndFailures);
        runner.add("task pool: cancel queued and running", testCancelQueuedAndRunning);
        runner.add("task pool: progress merged", testProgressMerged);
        runner.add("task pool: destructor cancels and joins", testDestructorCancels);
    }
}
