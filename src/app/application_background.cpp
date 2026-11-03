#include "gran/app/internal/app_state.hpp"

#include <memory>
#include <utility>

namespace gran
{
    namespace
    {
        // Отправка задач пула в поток интерфейса через Application::post (функтор вместо лямбды) / Sending the pool tasks to the UI thread through Application::post (a functor instead of a lambda)
        struct PostToApplication
        {
            Application* application;

            void operator()(app::UiTask task) const
            {
                application->post(std::move(task));
            }
        };

        // Пул создаётся при первой задаче: программа без фоновых задач не держит лишних потоков / The pool is created on the first task: a program without background tasks keeps no extra threads
        app::TaskPool& poolOf(Application::State& state, Application& application)
        {
            if (!state.pool)
                state.pool = std::make_unique<app::TaskPool>(PostToApplication{&application}, state.options.taskThreads);
            return *state.pool;
        }
    }

    // Работа — в пул; done и progress придут в поток интерфейса / The work goes to the pool; done and progress come to the UI thread
    TaskId Application::runTask(BackgroundWork work, TaskDone done, TaskProgress progress)
    {
        return poolOf(*_state, *this).submit(std::move(work), std::move(done), std::move(progress));
    }

    // Без пула задач нет — отменять нечего / Without a pool there are no tasks — nothing to cancel
    bool Application::cancelTask(TaskId id)
    {
        if (!_state->pool)
            return false;
        return _state->pool->cancel(id);
    }

    // Задачи, чей итог ещё не доставлен / The tasks whose outcome is not delivered yet
    std::size_t Application::activeTasks() const
    {
        if (!_state->pool)
            return 0;
        return _state->pool->active();
    }
}
