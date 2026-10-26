#include "gran/app/internal/app_state.hpp"

#include <utility>

namespace gran
{
    /*
    ** Замок держится и во время wake(): иначе run() мог бы обнулить wakeTarget и разрушить
    ** окно между проверкой указателя и вызовом. wake() не ждёт поток интерфейса
    ** (PostMessage), поэтому взаимной блокировки нет.
    **
    ** The lock is held during wake() too: otherwise run() could clear wakeTarget and destroy
    ** the window between the pointer check and the call. wake() does not wait for the UI thread
    ** (PostMessage), so there is no deadlock.
    */
    void Application::post(Task task)
    {
        std::lock_guard<std::mutex> lock(_state->taskMutex);

        _state->tasks.push_back(std::move(task));
        if (_state->wakeTarget != nullptr)
            _state->wakeTarget->wake();
    }

    namespace app
    {
        /*
        ** Очередь забирается целиком под замком, а задачи выполняются уже без него: задача
        ** может сама вызвать post (и не зависнуть), а фоновые потоки не ждут, пока она идёт.
        **
        ** The queue is taken entirely under the lock, and the tasks run without it: a task
        ** may call post itself (without hanging), and the background threads do not wait while it runs.
        */
        bool runTasks(Application::State& state)
        {
            std::vector<Task> batch;

            {
                std::lock_guard<std::mutex> lock(state.taskMutex);
                batch.swap(state.tasks);
            }
            for (Task& task : batch)
            {
                if (task)
                    task();
            }
            return !batch.empty();
        }

        // Смена окна-получателя под тем же замком, что и post / Changing the target window under the same lock as post
        void setWakeTarget(Application::State& state, platform::Window* window)
        {
            std::lock_guard<std::mutex> lock(state.taskMutex);

            state.wakeTarget = window;
        }
    }
}
