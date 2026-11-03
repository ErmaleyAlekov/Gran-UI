#include "gallery_actions.hpp"

#include <filesystem>
#include <string>

namespace gallery
{
    namespace
    {
        // Текст строки состояния programStatus (samples/program.xml) / The programStatus line text (samples/program.xml)
        void setStatus(gran::Application& application, const std::string& text)
        {
            gran::ui::Node* status = application.find("programStatus");

            if (status == nullptr)
                return;
            status->content.text = text;
            gran::ui::markDirty(*status, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
        }

        // Результат модального диалога — в строку состояния главного окна / The modal dialog result goes into the main window status line
        struct DialogClosed
        {
            gran::Application* application;

            void operator()(const std::string& result) const
            {
                std::string text = result;

                if (text.empty())
                    text = "закрыт крестиком";
                setStatus(*application, "Диалог: " + text);
            }
        };

        /*
        ** Модальное окно, подчинённое окну нажатой кнопки: из главного — диалог над главным,
        ** из диалога — следующий диалог над ним (блокирует оба). modal = false — окно-инструмент.
        **
        ** A modal window owned by the window of the pressed button: from the main one — a dialog above the main one,
        ** from a dialog — the next dialog above it (it blocks both). modal = false — a tool window.
        */
        struct OpenOwned
        {
            gran::Application* application;
            std::string markup;
            bool modal;

            void operator()(gran::ui::Node& node) const
            {
                gran::WindowOptions options;

                options.owner = &node;
                options.modal = modal;
                if (modal)
                    options.onClosed = DialogClosed{application};
                application->openWindow(markup, options);
            }
        };
    }

    // Действия samples/program.xml и его диалога / The samples/program.xml and its dialog actions
    void registerProgram(gran::Application& application, const std::string& markup)
    {
        std::filesystem::path folder = std::filesystem::path(markup).parent_path();

        application.onAction("programDialog", OpenOwned{&application, (folder / "program_dialog.xml").string(), true});
        application.onAction("programTool", OpenOwned{&application, (folder / "tool_window.xml").string(), false});
    }
}
