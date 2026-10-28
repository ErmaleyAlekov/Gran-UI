#include "gallery_actions.hpp"

#include <string>

namespace gallery
{
    namespace
    {
        // Шаг полосы timerProgress: +1 до максимума, затем снова с минимума / A timerProgress bar step: +1 up to the maximum, then from the minimum again
        struct ProgressTick
        {
            gran::Application* application;

            void operator()() const
            {
                gran::ui::Node* bar = application->find("timerProgress");

                if (bar == nullptr)
                    return;
                float next = bar->content.value + 1.0f;
                if (next > bar->content.maximum)
                    next = bar->content.minimum;
                application->setValue(*bar, next);
            }
        };

        // Счётчик секунд в Text elapsed; состояние живёт в самом обработчике / A seconds counter in the elapsed Text; the state lives in the handler itself
        struct ElapsedTick
        {
            gran::Application* application;
            int seconds = 0;

            void operator()()
            {
                gran::ui::Node* text = application->find("elapsed");

                ++seconds;
                if (text == nullptr)
                    return;
                text->content.text = "Прошло: " + std::to_string(seconds) + " с";
                gran::ui::markDirty(*text, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
            }
        };

        // Текст в строку состояния status / A text into the status line status
        struct StatusText
        {
            gran::Application* application;
            std::string text;

            void operator()() const
            {
                gran::ui::Node* status = application->find("status");

                if (status == nullptr)
                    return;
                status->content.text = text;
                gran::ui::markDirty(*status, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
            }
        };

        // Действие delayed: сразу — «ждём», через 2 секунды — «сработал» / The delayed action: at once — "waiting", in 2 seconds — "fired"
        struct DelayedAction
        {
            gran::Application* application;

            void operator()(gran::ui::Node& node) const
            {
                StatusText{application, "Ждём 2 секунды…"}();
                application->setTimeout(2.0, StatusText{application, "Таймер сработал"});
                (void)node;
            }
        };
    }

    // Периодические таймеры — только если в разметке есть их узлы: иначе окно зря перерисовывалось бы в простое
    // Periodic timers only if the markup has their nodes: otherwise the window would be redrawn for nothing when idle
    void registerTimers(gran::Application& application)
    {
        if (application.find("timerProgress") != nullptr)
            application.setInterval(0.05, ProgressTick{&application});
        if (application.find("elapsed") != nullptr)
            application.setInterval(1.0, ElapsedTick{&application});
        application.onAction("delayed", DelayedAction{&application});
    }
}
