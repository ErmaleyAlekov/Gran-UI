#include "gallery_actions.hpp"

#include <string>

namespace gallery
{
    namespace
    {
        // Строка состояния status (если она есть в разметке) / The status line status (if the markup has it)
        struct StatusAction
        {
            gran::Application* application;
            std::string prefix;

            void operator()(gran::ui::Node& node) const
            {
                gran::ui::Node* status = application->find("status");

                if (status == nullptr)
                    return;
                status->content.text = prefix + node.content.text;
                gran::ui::markDirty(*status, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
            }
        };

        // Действие «Сохранить»: имя из поля ввода — в строку состояния / The "Save" action: the name from the input goes into the status line
        struct SaveAction
        {
            gran::Application* application;

            void operator()(gran::ui::Node& node) const
            {
                gran::ui::Node* name = application->find("name");

                if (name != nullptr)
                    StatusAction{application, "Сохранено: "}(*name);
                (void)node;
            }
        };

        // Действие «more»: меню moreMenu (samples/popups.xml) под нажатой кнопкой / The "more" action: the moreMenu menu (samples/popups.xml) under the pressed button
        struct MenuAction
        {
            gran::Application* application;

            void operator()(gran::ui::Node& node) const
            {
                gran::ui::Node* menu = application->find("moreMenu");

                if (menu != nullptr)
                    application->showPopup(*menu, node);
            }
        };

        // Кнопка внутри диалога закрывает ближайший диалог-предок / A button inside a dialog closes the nearest dialog ancestor
        struct CloseDialogAction
        {
            gran::Application* application;

            void operator()(gran::ui::Node& node) const
            {
                for (gran::ui::Node* current = &node; current != nullptr; current = current->parent)
                {
                    if (current->kind != gran::ui::ElementKind::DIALOG)
                        continue;
                    application->hideDialog(*current);
                    return;
                }
            }
        };
    }

    // Имена действий совпадают с onClick/onChange в samples/*.xml / The action names match onClick/onChange in samples/*.xml
    void registerActions(gran::Application& application)
    {
        application.onAction("save", SaveAction{&application});
        application.onAction("more", MenuAction{&application});
        application.onAction("command", StatusAction{&application, "Команда: "});
        application.onAction("choice", StatusAction{&application, "Выбрано: "});
        application.onAction("closeDialog", CloseDialogAction{&application});
    }
}
