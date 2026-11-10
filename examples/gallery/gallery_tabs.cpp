#include "gallery_actions.hpp"

#include <string>

namespace gallery
{
    namespace
    {
        // Строка состояния status (если есть): текст и перекомпоновка / The status line status (if any): the text and a relayout
        void showStatus(gran::Application& application, const std::string& text)
        {
            gran::ui::Node* status = application.find("status");

            if (status == nullptr)
                return;
            status->content.text = text;
            gran::ui::markDirty(*status, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
        }

        // onClose вкладок документов: страница закрывается сразу (настоящая программа спросила бы о сохранении)
        // The document tabs onClose: the page closes at once (a real program would ask about saving)
        struct CloseTab
        {
            gran::Application* application;

            void operator()(gran::ui::Node& page) const
            {
                std::string title = page.content.text;

                application->remove(page);
                showStatus(*application, "Закрыта вкладка: " + title);
            }
        };

        // onReorder: вкладку перетащили на новое место / onReorder: the tab was dragged to a new place
        struct TabMoved
        {
            gran::Application* application;

            void operator()(gran::ui::Node& page) const
            {
                showStatus(*application, "Вкладка перенесена: " + page.content.text);
            }
        };
    }

    // Действия вкладок документов samples/editor_frame.xml / The document tab actions of samples/editor_frame.xml
    void registerTabs(gran::Application& application)
    {
        application.onAction("closeTab", CloseTab{&application});
        application.onAction("tabMoved", TabMoved{&application});
    }
}
