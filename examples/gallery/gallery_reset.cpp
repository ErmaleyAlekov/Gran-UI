#include "gallery_events.hpp"

namespace gallery
{
    namespace
    {
        // Кнопка не знает, кто сбрасывает доску: она шлёт своё событие, его ловит on="boardReset:…" у доски
        // The button does not know who resets the board: it sends a custom event, on="boardReset:…" at the board catches it
        struct ResetAction
        {
            gran::Application* application;

            void operator()(gran::ui::Node& node) const
            {
                if (!application->dispatchEvent(node, "boardReset"))
                    setText(*application, "status", "Сброс отменён");
            }
        };

        // Обработчик доски: колонки — как в начале примера / The board handler: the columns are as at the sample start
        struct ResetBoard
        {
            gran::Application* application;

            void operator()(gran::ui::Node&, gran::ui::UiEvent& event) const
            {
                gran::data::ListModel& todo = application->model("todo");
                gran::data::ListModel& done = application->model("done");

                todo.clear();
                done.clear();
                todo.append({{"title", "Макет окна настроек"}});
                todo.append({{"title", "Горячие клавиши"}});
                todo.append({{"title", "Отмена и повтор"}});
                done.append({{"title", "Модель событий"}});
                setText(*application, "status", "Доска сброшена");
                event.stopPropagation();
            }
        };
    }

    // Узла нет в этой разметке — ничего / The node is not in this markup — nothing
    void setText(gran::Application& application, const char* id, const std::string& text)
    {
        gran::ui::Node* node = application.find(id);

        if (node == nullptr)
            return;
        node->content.text = text;
        gran::ui::markDirty(*node, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
    }

    // reset — действие кнопки, resetBoard — обработчик своего события / reset is the button action, resetBoard is the custom event handler
    void registerReset(gran::Application& application)
    {
        application.onAction("reset", ResetAction{&application});
        application.onEvent("resetBoard", ResetBoard{&application});
    }
}
