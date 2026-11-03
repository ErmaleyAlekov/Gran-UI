#ifndef GRAN_APP_INTERNAL_TASK_POOL_HPP
#define GRAN_APP_INTERNAL_TASK_POOL_HPP

#include "gran/app/background_task.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace gran::app
{
    class TaskPool;

    // Задача для потока интерфейса (как Application::Task) / A task for the UI thread (as Application::Task)
    using UiTask = std::function<void()>;

    // Отправка задачи в поток интерфейса (Application::post) — из любого потока / Sending a task to the UI thread (Application::post) — from any thread
    using TaskPoster = std::function<void(UiTask)>;

    /*
    ** Одна фоновая задача. Общая (shared_ptr) для пула, потока работы и доставки итога в поток
    ** интерфейса. outcome пишет поток работы до отправки итога, читает поток интерфейса после:
    ** передачу упорядочивает замок очереди post. finished меняет только поток интерфейса.
    **
    ** One background task. Shared (shared_ptr) by the pool, the work thread and the delivery of the outcome to the UI
    ** thread. The work thread writes outcome before sending the outcome, the UI thread reads it after:
    ** the post queue lock orders the handover. Only the UI thread changes finished.
    */
    struct TaskRecord : std::enable_shared_from_this<TaskRecord>
    {
        TaskPool* pool = nullptr;
        BackgroundWork work;
        TaskDone done;
        TaskProgress progress;
        std::atomic<bool> cancel{false};
        TaskOutcome outcome;
        // Последний прогресс под замком; progressQueued — его доставка уже стоит в очереди интерфейса
        // The latest progress under the lock; progressQueued — its delivery already stands in the UI queue
        std::mutex progressMutex;
        TaskProgressInfo latest;
        bool progressQueued = false;
        bool finished = false;
    };

    /*
    ** Пул потоков для фоновых задач (паттерн Thread Pool + Active Object): общая очередь FIFO,
    ** потоки создаются при первой задаче. Итог и прогресс уходят в поток интерфейса через poster,
    ** поэтому обработчики меняют дерево без своих замков. Деструктор отменяет всё и ждёт потоки:
    ** работа обязана проверять cancelled().
    **
    ** A thread pool for background tasks (the Thread Pool + Active Object patterns): a shared FIFO queue,
    ** the threads are created on the first task. The outcome and the progress go to the UI thread through poster,
    ** so the handlers change the tree without locks of their own. The destructor cancels everything and waits for the threads:
    ** the work must check cancelled().
    */
    class TaskPool
    {
    public:
        // ---- task_pool.cpp ----

        // threads — число потоков (0 — ядра − 1, от 1 до 8) / threads is the thread count (0 — cores − 1, from 1 to 8)
        TaskPool(TaskPoster poster, unsigned threads);
        ~TaskPool();
        TaskPool(const TaskPool&) = delete;
        TaskPool& operator=(const TaskPool&) = delete;

        // Поставить задачу в очередь; номер — для cancel / Put a task into the queue; the number is for cancel
        TaskId submit(BackgroundWork work, TaskDone done, TaskProgress progress);

        // Попросить отменить; false — задачи нет (итог уже доставлен или номер чужой) / Ask to cancel; false — no such task (the outcome is delivered or the number is foreign)
        bool cancel(TaskId id);

        // ---- task_pool_run.cpp ----

        // Цикл потока пула: брать задачи, пока пул жив / The pool thread loop: take tasks while the pool lives
        void workerLoop();

        // Отправить итог в поток интерфейса (из потока работы) / Send the outcome to the UI thread (from the work thread)
        void finish(const std::shared_ptr<TaskRecord>& record) noexcept;

        // Отправить в поток интерфейса произвольную задачу (доставка прогресса) / Send an arbitrary task to the UI thread (the progress delivery)
        void post(UiTask task) noexcept;

        // Итог доставлен: задача больше не числится (поток интерфейса) / The outcome is delivered: the task is no longer counted (the UI thread)
        void forget(TaskId id);

        // Задачи в очереди, в работе и с недоставленным итогом / The tasks in the queue, at work and with an undelivered outcome
        std::size_t active() const;

        // Есть ли работа для потока или пора выходить (условие ожидания, под замком) / Whether there is work for a thread or it is time to leave (the wait condition, under the lock)
        bool ready() const noexcept;

    private:
        // ---- task_pool.cpp ----

        // Потоки — при первой задаче (под замком) / The threads — on the first task (under the lock)
        void start();

        TaskPoster _poster;
        unsigned _threadCount;
        mutable std::mutex _mutex;
        std::condition_variable _wake;
        std::deque<std::shared_ptr<TaskRecord>> _queue;
        std::map<TaskId, std::shared_ptr<TaskRecord>> _active;
        // Потоки объявлены последними: их join в деструкторе идёт, пока очередь и замок ещё живы
        // The threads are declared last: their join in the destructor goes while the queue and the lock are still alive
        std::vector<std::thread> _threads;
        TaskId _lastId = 0;
        bool _stopping = false;
    };

    // ---- task_worker.cpp ----

    // Выполнить работу задачи: исключение — FAILED с текстом, отмена — CANCELLED / Run the task work: an exception is FAILED with a text, a cancellation is CANCELLED
    void runRecord(TaskRecord& record) noexcept;
}

#endif
