#include "gran/app/internal/task_pool.hpp"

#include <utility>

namespace gran::app
{
    namespace
    {
        /*
        ** Доставка итога в потоке интерфейса: задача снимается с учёта пула, прогресс больше
        ** не вызывается, затем done получает итог (исключение done уходит в цикл run, как у post).
        **
        ** The outcome delivery on the UI thread: the task is removed from the pool accounting, the progress is no longer
        ** called, then done gets the outcome (a done exception goes to the run loop, as for post).
        */
        struct DeliverDone
        {
            std::shared_ptr<TaskRecord> record;

            void operator()() const
            {
                record->finished = true;
                record->pool->forget(record->outcome.id);
                if (record->done)
                    record->done(record->outcome);
            }
        };

        // Условие ожидания потока (функтор вместо лямбды) / The thread wait condition (a functor instead of a lambda)
        struct WaitReady
        {
            const TaskPool* pool;

            bool operator()() const
            {
                return pool->ready();
            }
        };
    }

    /*
    ** Поток берёт задачу под замком, выполняет её без замка и отправляет итог. Остановка пула
    ** выводит из цикла сразу: задачи, оставшиеся в очереди, не выполняются.
    **
    ** The thread takes a task under the lock, runs it without the lock and sends the outcome. Stopping the pool
    ** leaves the loop at once: the tasks left in the queue do not run.
    */
    void TaskPool::workerLoop()
    {
        for (;;)
        {
            std::shared_ptr<TaskRecord> record;

            {
                std::unique_lock<std::mutex> lock(_mutex);
                _wake.wait(lock, WaitReady{this});
                if (_stopping)
                    return;
                record = std::move(_queue.front());
                _queue.pop_front();
            }
            runRecord(*record);
            finish(record);
        }
    }

    // Итог — в поток интерфейса; копия записи живёт в доставке / The outcome goes to the UI thread; a record copy lives in the delivery
    void TaskPool::finish(const std::shared_ptr<TaskRecord>& record) noexcept
    {
        post(DeliverDone{record});
    }

    /*
    ** Нехватка памяти при постановке в очередь не должна убить поток пула: доставка теряется,
    ** задача остаётся числиться активной до выхода из приложения.
    **
    ** Running out of memory while queuing must not kill the pool thread: the delivery is lost,
    ** the task stays counted as active until the application exit.
    */
    void TaskPool::post(UiTask task) noexcept
    {
        try
        {
            _poster(std::move(task));
        }
        catch (...)
        {
        }
    }
}
