#include "gallery_actions.hpp"

#include "gran/log/log.hpp"

#include <string>

namespace gallery
{
    namespace
    {
        // Текст узла по id / The node text by id
        void setInfo(gran::Application& application, const char* id, const std::string& text)
        {
            gran::ui::Node* node = application.find(id);

            if (node == nullptr)
                return;
            node->content.text = text;
            gran::ui::markDirty(*node, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
        }

        // «Записать в журнал»: строка категории gallery и путь файла под кнопкой / "Write to the log": a gallery category line and the file path under the button
        struct LogButton
        {
            gran::Application* application;

            void operator()(gran::ui::Node&) const
            {
                gran::log::write(gran::log::Level::INFO, "gallery", "запись из примера samples/program.xml");
                setInfo(*application, "logInfo", "Журнал: " + gran::log::filePath());
            }
        };

        /*
        ** «Упасть»: запись по нулевому адресу. Обработчик сбоя пишет отчёт (на Windows ещё минидамп),
        ** при следующем запуске галереи его путь появится в разделе примера.
        **
        ** "Crash": a write to the null address. The crash handler writes a report (on Windows also a minidump),
        ** on the next gallery start its path appears in the sample section.
        */
        struct CrashButton
        {
            void operator()(gran::ui::Node&) const
            {
                gran::log::write(gran::log::Level::WARNING, "gallery", "намеренное падение по кнопке примера");
                *static_cast<volatile int*>(nullptr) = 1;
            }
        };
    }

    // Кнопки журнала и сведения о прошлом сбое samples/program.xml / The log buttons and the previous crash information of samples/program.xml
    void registerLog(gran::Application& application)
    {
        application.onAction("logWrite", LogButton{&application});
        application.onAction("crashNow", CrashButton{});
        if (!gran::log::filePath().empty())
            setInfo(application, "logInfo", "Журнал: " + gran::log::filePath());
        if (!application.previousCrash().empty())
            setInfo(application, "crashInfo", "Отчёт о прошлом сбое: " + application.previousCrash());
    }
}
