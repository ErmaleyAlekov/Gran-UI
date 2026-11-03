#include "gran/capi/internal/bridge.hpp"

namespace
{
    // Работа языковой стороны: контекст передаётся непрозрачным gran_task / The language side work: the context goes as an opaque gran_task
    struct CWork
    {
        gran_work_fn callback;
        void* user;

        void operator()(gran::TaskContext& context) const
        {
            callback(user, reinterpret_cast<gran_task*>(&context));
        }
    };

    // Итог: состояние GRAN_TASK_* совпадает с порядком TaskStatus / The outcome: the GRAN_TASK_* status matches the TaskStatus order
    struct CDone
    {
        gran_task_done_fn callback;
        void* user;

        void operator()(gran::TaskOutcome& outcome) const
        {
            callback(user, outcome.id, static_cast<int32_t>(outcome.status), outcome.error.c_str());
        }
    };

    // Прогресс: строка действительна только во время вызова / The progress: the string is valid only during the call
    struct CProgress
    {
        gran_task_progress_fn callback;
        void* user;

        void operator()(const gran::TaskProgressInfo& info) const
        {
            callback(user, info.id, info.fraction, info.text.c_str());
        }
    };
}

// work обязателен, done и progress — по желанию; size защищает от старой структуры / work is required, done and progress are optional; size protects against an older structure
gran_status gran_app_run_task(gran_app* app, const gran_background* task, uint64_t* result)
{
    if (result != nullptr) *result = 0;
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    gran::capi::require(task != nullptr && result != nullptr && task->work != nullptr);
    gran::capi::require(task->size >= sizeof(gran_background));
    gran::TaskDone done;
    gran::TaskProgress progress;
    if (task->done != nullptr)
        done = CDone{task->done, task->user};
    if (task->progress != nullptr)
        progress = CProgress{task->progress, task->user};
    *result = app->application.runTask(CWork{task->work, task->user}, done, progress);
    GRAN_CAPI_END
}

// found (по желанию) — была ли такая задача / found (optional) — whether such a task existed
gran_status gran_app_cancel_task(gran_app* app, uint64_t task, int32_t* found)
{
    if (found != nullptr) *found = 0;
    GRAN_CAPI_TRY
    gran::capi::checkApp(app);
    bool cancelled = app->application.cancelTask(task);
    if (found != nullptr) *found = static_cast<int32_t>(cancelled);
    GRAN_CAPI_END
}
