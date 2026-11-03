#ifndef GRAN_APP_BACKGROUND_TASK_HPP
#define GRAN_APP_BACKGROUND_TASK_HPP

#include <any>
#include <cstdint>
#include <functional>
#include <string>

namespace gran::app
{
    struct TaskRecord;
}

namespace gran
{
    // Номер фоновой задачи (Application::runTask); 0 — «нет задачи» / A background task number (Application::runTask); 0 — "no task"
    using TaskId = std::uint64_t;

    // Чем кончилась фоновая задача: выполнена, отменена, сбой (исключение в работе) / How a background task ended: done, cancelled, failed (an exception in the work)
    enum class TaskStatus : std::uint8_t { DONE, CANCELLED, FAILED };

    /*
    ** Связь работы с пулом задач: отмена, прогресс и результат. Создаётся пулом на время вызова
    ** работы и действительна только внутри неё; методы вызываются из потока работы.
    ** Работа не трогает дерево интерфейса: всё, что меняет окно, делают обработчики done и progress
    ** в потоке интерфейса.
    **
    ** The link of a work to the task pool: cancellation, progress and the result. The pool creates it for the time of the work
    ** call and it is valid only inside it; the methods are called from the work thread.
    ** The work does not touch the UI tree: everything that changes a window is done by the done and progress handlers
    ** on the UI thread.
    */
    class TaskContext
    {
    public:
        // ---- task_context.cpp ----
        explicit TaskContext(app::TaskRecord& record) noexcept;

        // Номер задачи / The task number
        TaskId id() const noexcept;

        // Просили ли отменить (cancelTask или выход из приложения): работа должна закончиться как можно скорее
        // Whether cancelling was asked (cancelTask or the application exit): the work must finish as soon as possible
        bool cancelled() const noexcept;

        // Результат для TaskOutcome::result (любое копируемое значение) / The result for TaskOutcome::result (any copyable value)
        void setResult(std::any value);

        // Сбой с текстом (для обёрток языков, где исключение не пересекает C ABI) / A failure with a text (for language wrappers where an exception does not cross the C ABI)
        void fail(const std::string& message);

        // ---- task_progress.cpp ----

        /*
        ** Прогресс 0…1 и подпись. Частые вызовы сливаются: обработчик progress в потоке интерфейса
        ** получает последнее значение, а не каждое; после done прогресс не приходит.
        **
        ** The progress 0…1 and a caption. Frequent calls are merged: the progress handler on the UI thread
        ** gets the latest value, not each one; no progress comes after done.
        */
        void progress(double fraction, const std::string& text = std::string());

    private:
        app::TaskRecord& _record;
    };

    // Итог задачи для обработчика done: номер, состояние, текст сбоя и результат setResult / The task outcome for the done handler: the number, the status, the failure text and the setResult result
    struct TaskOutcome
    {
        TaskId id = 0;
        TaskStatus status = TaskStatus::DONE;
        std::string error;
        std::any result;
    };

    // Прогресс задачи для обработчика progress / The task progress for the progress handler
    struct TaskProgressInfo
    {
        TaskId id = 0;
        double fraction = 0.0;
        std::string text;
    };

    // Работа в потоке пула; обработчики итога и прогресса — в потоке интерфейса / The work on a pool thread; the outcome and progress handlers — on the UI thread
    using BackgroundWork = std::function<void(TaskContext& context)>;
    using TaskDone = std::function<void(TaskOutcome& outcome)>;
    using TaskProgress = std::function<void(const TaskProgressInfo& info)>;
}

#endif
