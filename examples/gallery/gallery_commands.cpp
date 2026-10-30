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

        // «Сохранить»: текущая позиция стека отмены — «чистая», «*» в заголовке пропадает / "Save": the current undo stack position is clean, "*" in the title disappears
        struct SaveDoc
        {
            gran::Application* application;

            void operator()(gran::ui::Node&) const
            {
                application->undoStack().setClean();
                setText(*application, "status", "Команда «Сохранить» (Ctrl+S): изменения сохранены");
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
        application.onAction("cmdSave", SaveDoc{&application});
        application.onAction("cmdSaveAs", CommandStatus{&application, "Команда «Сохранить как…» (Ctrl+Shift+S)"});
        application.onAction("cmdRefresh", CommandStatus{&application, "Команда «Обновить» (F5 или Ctrl+R)"});
        application.onAction("toggleSave", ToggleSave{&application});
        registerNotes(application);
    }
}
