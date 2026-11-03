#include "gran/capi/internal/bridge.hpp"

namespace
{
    // gran_task — это TaskContext потока работы / gran_task is the work thread TaskContext
    gran::TaskContext& contextOf(gran_task* task)
    {
        gran::capi::require(task != nullptr);
        return *reinterpret_cast<gran::TaskContext*>(task);
    }
}

// Из потока работы; NULL — «не отменена» / From the work thread; NULL — "not cancelled"
int32_t gran_task_cancelled(const gran_task* task)
{
    if (task == nullptr)
        return 0;
    return static_cast<int32_t>(reinterpret_cast<const gran::TaskContext*>(task)->cancelled());
}

// Прогресс сливается: в поток UI доходит последнее значение / The progress is merged: the latest value reaches the UI thread
gran_status gran_task_progress(gran_task* task, double fraction, const char* text)
{
    GRAN_CAPI_TRY
    contextOf(task).progress(fraction, gran::capi::text(text));
    GRAN_CAPI_END
}

// Сбой без исключения (исключения языка не пересекают C ABI) / A failure without an exception (the language exceptions do not cross the C ABI)
gran_status gran_task_fail(gran_task* task, const char* message)
{
    GRAN_CAPI_TRY
    contextOf(task).fail(gran::capi::text(message));
    GRAN_CAPI_END
}
