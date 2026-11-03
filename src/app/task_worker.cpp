#include "gran/app/internal/task_pool.hpp"
#include "gran/core/i18n.hpp"

#include <exception>

namespace gran::app
{
    /*
    ** Работа выполняется, если её не отменили до начала. Исключение не выходит из потока пула:
    ** оно становится состоянием FAILED с текстом. Отмена во время работы даёт CANCELLED
    ** (результат отбрасывается), если работа сама не сообщила о сбое.
    **
    ** The work runs unless it was cancelled before the start. An exception does not leave the pool thread:
    ** it becomes the FAILED status with a text. A cancellation during the work gives CANCELLED
    ** (the result is dropped) unless the work itself reported a failure.
    */
    void runRecord(TaskRecord& record) noexcept
    {
        TaskContext context(record);

        try
        {
            if (!record.cancel.load() && record.work)
                record.work(context);
        }
        catch (const std::exception& error)
        {
            context.fail(error.what());
        }
        catch (...)
        {
            context.fail(i18n::format(i18n::Message::TASK_UNKNOWN_ERROR));
        }
        if (record.cancel.load() && record.outcome.status != TaskStatus::FAILED)
            record.outcome.status = TaskStatus::CANCELLED;
        // Результат есть только у выполненной задачи (setResult после fail тоже отбрасывается)
        // Only a done task has a result (setResult after fail is dropped too)
        if (record.outcome.status != TaskStatus::DONE)
            record.outcome.result.reset();
    }

    // Отмена — только флаг: работа заметит его сама, итог придёт с CANCELLED / Cancelling is only a flag: the work notices it itself, the outcome comes with CANCELLED
    bool TaskPool::cancel(TaskId id)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto found = _active.find(id);

        if (found == _active.end())
            return false;
        found->second->cancel.store(true);
        return true;
    }

    // Итог доставлен — задача больше не активна / The outcome is delivered — the task is no longer active
    void TaskPool::forget(TaskId id)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        _active.erase(id);
    }

    // Задачи в очереди, в работе и ждущие доставки итога / The tasks in the queue, at work and waiting for the outcome delivery
    std::size_t TaskPool::active() const
    {
        std::lock_guard<std::mutex> lock(_mutex);

        return _active.size();
    }

    // Вызывается под замком из ожидания потока / Called under the lock from the thread wait
    bool TaskPool::ready() const noexcept
    {
        return _stopping || !_queue.empty();
    }
}
