#include "gallery_actions.hpp"

#include <memory>
#include <string>

namespace gallery
{
    namespace
    {
        // Номер идущей задачи примера (0 — задачи нет); общий для кнопок и обработчиков / The running sample task number (0 — no task); shared by the buttons and the handlers
        struct TaskDemo
        {
            gran::TaskId current = 0;
        };

        // Текст узла по id / The node text by id
        void setText(gran::Application& application, const char* id, const std::string& text)
        {
            gran::ui::Node* node = application.find(id);

            if (node == nullptr)
                return;
            node->content.text = text;
            gran::ui::markDirty(*node, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
        }

        // Прогресс — в полосу и подпись (поток интерфейса) / The progress goes into the bar and the caption (the UI thread)
        struct ShowProgress
        {
            gran::Application* application;

            void operator()(const gran::TaskProgressInfo& info) const
            {
                gran::ui::Node* bar = application->find("taskProgress");

                if (bar != nullptr)
                    application->setValue(*bar, static_cast<float>(info.fraction * 100.0));
                setText(*application, "taskStatus", info.text);
            }
        };

        // Итог: результат, отмена или сбой — в подпись; задача больше не идёт / The outcome: the result, the cancellation or the failure goes into the caption; the task no longer runs
        struct ShowOutcome
        {
            gran::Application* application;
            std::shared_ptr<TaskDemo> demo;

            void operator()(gran::TaskOutcome& outcome) const
            {
                demo->current = 0;
                if (outcome.status == gran::TaskStatus::DONE)
                    setText(*application, "taskStatus", std::any_cast<std::string>(outcome.result));
                else if (outcome.status == gran::TaskStatus::CANCELLED)
                    setText(*application, "taskStatus", "Отменено");
                else
                    setText(*application, "taskStatus", "Сбой: " + outcome.error);
            }
        };

        // «Запустить» — новая задача, если прежняя закончилась; «Отменить» — флаг отмены / "Start" — a new task if the former one finished; "Cancel" — the cancel flag
        struct TaskButton
        {
            gran::Application* application;
            std::shared_ptr<TaskDemo> demo;
            bool start;

            void operator()(gran::ui::Node&) const
            {
                if (!start)
                {
                    if (demo->current != 0)
                        application->cancelTask(demo->current);
                    return;
                }
                if (demo->current != 0)
                    return;
                demo->current = application->runTask(slowWork, ShowOutcome{application, demo}, ShowProgress{application});
            }
        };
    }

    // Кнопки фоновой задачи samples/program.xml / The background task buttons of samples/program.xml
    void registerTasks(gran::Application& application)
    {
        std::shared_ptr<TaskDemo> demo = std::make_shared<TaskDemo>();

        application.onAction("taskStart", TaskButton{&application, demo, true});
        application.onAction("taskCancel", TaskButton{&application, demo, false});
    }
}
