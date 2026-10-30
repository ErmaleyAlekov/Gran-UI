#include "gallery_events.hpp"

namespace gallery
{
    namespace
    {
        // Действие команды пишет в строку состояния, что выполнено / A command action writes what was done into the status line
        struct CommandStatus
        {
            gran::Application* application;
            const char* text;

            void operator()(gran::ui::Node&) const
            {
                setText(*application, "status", text);
            }
        };

        // Del на заметке: узел исполнителя — заметка в фокусе (область focus внутри notes) / Del on a note: the running node is the focused note (the focus scope inside notes)
        struct DeleteNote
        {
            gran::Application* application;

            void operator()(gran::ui::Node& node) const
            {
                if (node.kind != gran::ui::ElementKind::BUTTON || node.parent == nullptr || node.parent->id != "notes")
                    return;
                setText(*application, "status", "Заметка «" + node.content.text + "» удалена");
                application->remove(node);
            }
        };

        // Флажок выключает «Сохранить» и «Сохранить как…»: кнопка, пункты меню и сочетания — вместе
        // The check box disables "Save" and "Save as…": the button, the menu items and the shortcuts — together
        struct ToggleSave
        {
            gran::Application* application;

            void operator()(gran::ui::Node& box) const
            {
                application->setCommandEnabled("saveDoc", box.content.checked);
                application->setCommandEnabled("saveAs", box.content.checked);
            }
        };
    }

    // Имена из samples/commands.xml / The names from samples/commands.xml
    void registerCommands(gran::Application& application)
    {
        application.onAction("cmdNew", CommandStatus{&application, "Команда «Новый» (Ctrl+N)"});
        application.onAction("cmdOpen", CommandStatus{&application, "Команда «Открыть…» (Ctrl+O)"});
        application.onAction("cmdSave", CommandStatus{&application, "Команда «Сохранить» (Ctrl+S)"});
        application.onAction("cmdSaveAs", CommandStatus{&application, "Команда «Сохранить как…» (Ctrl+Shift+S)"});
        application.onAction("cmdRefresh", CommandStatus{&application, "Команда «Обновить» (F5 или Ctrl+R)"});
        application.onAction("cmdDelete", DeleteNote{&application});
        application.onAction("toggleSave", ToggleSave{&application});
    }
}
