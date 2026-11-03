#include "gran/app/internal/task_pool.hpp"

#include <memory>
#include <utility>

namespace gran
{
    namespace
    {
        /*
        ** Доставка прогресса в поток интерфейса: берётся последнее значение, флаг очереди сбрасывается
        ** (следующий вызов progress поставит новую доставку). После итога прогресс не вызывается.
        **
        ** The progress delivery to the UI thread: the latest value is taken, the queue flag is reset
        ** (the next progress call puts a new delivery). After the outcome the progress is not called.
        */
        struct DeliverProgress
        {
            std::shared_ptr<app::TaskRecord> record;

            void operator()() const
            {
                TaskProgressInfo info;

                {
                    std::lock_guard<std::mutex> lock(record->progressMutex);
                    info = record->latest;
                    record->progressQueued = false;
                }
                if (!record->finished && record->progress)
                    record->progress(info);
            }
        };
    }

    /*
    ** Значение запоминается под замком; доставка ставится в очередь, только если её там ещё нет —
    ** тысяча вызовов за кадр даёт один вызов обработчика. Без обработчика progress ничего не отправляется.
    ** Запись задачи создана через make_shared — её владеющая копия уходит в доставку.
    **
    ** The value is remembered under the lock; the delivery is queued only if it is not there yet —
    ** a thousand calls per frame give one handler call. Without a progress handler nothing is sent.
    ** The task record is created by make_shared — its owning copy goes into the delivery.
    */
    void TaskContext::progress(double fraction, const std::string& text)
    {
        bool queue = false;

        {
            std::lock_guard<std::mutex> lock(_record.progressMutex);
            _record.latest.id = _record.outcome.id;
            _record.latest.fraction = fraction;
            _record.latest.text = text;
            if (_record.progress && !_record.progressQueued)
            {
                _record.progressQueued = true;
                queue = true;
            }
        }
        if (queue)
            _record.pool->post(DeliverProgress{_record.shared_from_this()});
    }
}
