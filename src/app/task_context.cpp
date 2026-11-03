#include "gran/app/internal/task_pool.hpp"

#include <utility>

namespace gran
{
    // Контекст лишь ссылается на запись задачи: живёт на стеке потока работы / The context only refers to the task record: it lives on the work thread stack
    TaskContext::TaskContext(app::TaskRecord& record) noexcept
        : _record(record)
    {
    }

    // Номер задан при постановке и больше не меняется / The number is set on submitting and never changes
    TaskId TaskContext::id() const noexcept
    {
        return _record.outcome.id;
    }

    // Флаг атомарный: cancelTask ставит его из потока интерфейса / The flag is atomic: cancelTask sets it from the UI thread
    bool TaskContext::cancelled() const noexcept
    {
        return _record.cancel.load(std::memory_order_relaxed);
    }

    // Итог читается потоком интерфейса только после работы / The outcome is read by the UI thread only after the work
    void TaskContext::setResult(std::any value)
    {
        _record.outcome.result = std::move(value);
    }

    // Сбой без исключения: состояние FAILED, результат отбрасывается / A failure without an exception: the FAILED status, the result is dropped
    void TaskContext::fail(const std::string& message)
    {
        _record.outcome.status = TaskStatus::FAILED;
        _record.outcome.error = message;
        _record.outcome.result.reset();
    }
}
