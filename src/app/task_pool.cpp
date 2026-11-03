#include "gran/app/internal/task_pool.hpp"

#include <algorithm>
#include <utility>

namespace gran::app
{
    namespace
    {
        // Точка входа потока пула (функтор вместо лямбды) / The pool thread entry point (a functor instead of a lambda)
        struct PoolThread
        {
            TaskPool* pool;

            void operator()() const
            {
                pool->workerLoop();
            }
        };
    }

    /*
    ** Потоков по умолчанию — ядер на одно меньше (поток интерфейса тоже работает), но не меньше
    ** одного и не больше восьми: фоновые задачи программы — файлы, сеть, разбор, а не числодробилка.
    **
    ** The default thread count is one less than the cores (the UI thread works too), but not less than
    ** one and not more than eight: the program background tasks are files, network, parsing, not number crunching.
    */
    TaskPool::TaskPool(TaskPoster poster, unsigned threads)
        : _poster(std::move(poster)), _threadCount(threads)
    {
        unsigned cores = std::thread::hardware_concurrency();

        if (_threadCount != 0)
            return;
        _threadCount = 1;
        if (cores > 2)
            _threadCount = std::min(cores - 1, 8u);
    }

    /*
    ** Все задачи отменяются, потоки будятся и дожидаются: работа, проверяющая cancelled(),
    ** заканчивается быстро. Итоги, не доставленные до этого, уничтожаются без вызова done.
    **
    ** All the tasks are cancelled, the threads are woken and waited for: a work checking cancelled()
    ** ends quickly. The outcomes not delivered by then are destroyed without calling done.
    */
    TaskPool::~TaskPool()
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _stopping = true;
            for (std::pair<const TaskId, std::shared_ptr<TaskRecord>>& entry : _active)
                entry.second->cancel.store(true);
        }
        _wake.notify_all();
        for (std::thread& thread : _threads)
        {
            if (thread.joinable())
                thread.join();
        }
    }

    // Потоки создаются один раз; ошибка создания потока — std::system_error наружу из submit / The threads are created once; a thread creation error is std::system_error out of submit
    void TaskPool::start()
    {
        if (!_threads.empty())
            return;
        _threads.reserve(_threadCount);
        for (unsigned index = 0; index < _threadCount; ++index)
            _threads.emplace_back(PoolThread{this});
    }

    // Номера растут с 1 и не повторяются за жизнь пула / The numbers grow from 1 and never repeat in the pool lifetime
    TaskId TaskPool::submit(BackgroundWork work, TaskDone done, TaskProgress progress)
    {
        std::shared_ptr<TaskRecord> record = std::make_shared<TaskRecord>();

        record->pool = this;
        record->work = std::move(work);
        record->done = std::move(done);
        record->progress = std::move(progress);
        {
            std::lock_guard<std::mutex> lock(_mutex);
            record->outcome.id = ++_lastId;
            start();
            _queue.push_back(record);
            _active[record->outcome.id] = record;
        }
        _wake.notify_one();
        return record->outcome.id;
    }
}
