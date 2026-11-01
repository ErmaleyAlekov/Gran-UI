#include "gallery_actions.hpp"
#include "gallery_events.hpp"

namespace gallery
{
    namespace
    {
        /*
        ** Перенос записи карточки card в модель target: запись снимается с исходного списка и встаёт
        ** в конец целевого — привязки items сами удалят и построят узлы (card после этого недействителен).
        **
        ** Moving the card record into the target model: the record leaves the source list and goes
        ** to the end of the target one — the items bindings delete and build the nodes themselves (card is invalid after that).
        */
        void moveRecord(gran::Application& application, const gran::ui::Node& card, gran::data::ListModel& target)
        {
            gran::ItemRef item = application.itemOf(card);
            // Колонки доски — хранилища ListModel (не прокси): запись можно удалить / The board columns are ListModel storages (not proxies): a record can be removed
            gran::data::ListModel* source = dynamic_cast<gran::data::ListModel*>(item.model);

            if (source == nullptr || source == &target)
                return;
            gran::data::Record record = source->at(item.index);
            source->remove(item.index);
            target.append(record);
            setText(application, "status", "Перенесено: " + record["title"]);
        }

        // Другая колонка доски для модели model / The other board column for the model
        gran::data::ListModel& otherColumn(gran::Application& application, const gran::data::ListSource* model)
        {
            if (model == &application.model("todo"))
                return application.model("done");
            return application.model("todo");
        }

        // onDrop колонки: узел обработчика — колонка (её модель items), related — перетащенная карточка
        // The column onDrop: the handler node is the column (its items model), related is the dragged card
        struct DropTask
        {
            gran::Application* application;

            void operator()(gran::ui::Node& column, gran::ui::UiEvent& event) const
            {
                if (event.related != nullptr)
                    moveRecord(*application, *event.related, application->model(column.content.items));
            }
        };

        // Двойной щелчок по карточке переносит её в другую колонку / A double click on a card moves it into the other column
        struct FlipTask
        {
            gran::Application* application;

            void operator()(gran::ui::Node& card, gran::ui::UiEvent&) const
            {
                gran::ItemRef item = application->itemOf(card);

                if (item.model != nullptr)
                    moveRecord(*application, card, otherColumn(*application, item.model));
            }
        };
    }

    // Имена обработчиков совпадают с samples/events.xml / The handler names match samples/events.xml
    void registerEvents(gran::Application& application)
    {
        application.onEvent("moveTask", DropTask{&application});
        application.onEvent("flipTask", FlipTask{&application});
        registerReset(application);
        registerPointer(application);
        registerKeys(application);
        registerWeb(application);
        registerCommands(application);
    }
}
