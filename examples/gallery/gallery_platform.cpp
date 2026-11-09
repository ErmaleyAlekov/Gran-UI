#include "gallery_actions.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace gallery
{
    namespace
    {
        // Текст строки состояния platformStatus (samples/platform.xml) / The platformStatus line text (samples/platform.xml)
        void setStatus(gran::Application& application, const std::string& text)
        {
            gran::ui::Node* status = application.find("platformStatus");

            if (status == nullptr)
                return;
            status->content.text = text;
            gran::ui::markDirty(*status, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
        }

        // Диалоги файлов: kind — 0 открыть (несколько), 1 сохранить, 2 папка; выбор — в строку состояния
        // The file dialogs: kind — 0 open (several), 1 save, 2 folder; the choice goes into the status line
        struct FileAction
        {
            gran::Application* application;
            int kind;

            void operator()(gran::ui::Node& node) const
            {
                gran::FileDialogOptions options;
                std::vector<std::string> paths;

                options.filters = {{"Картинки", "*.png;*.jpg;*.jpeg;*.svg"}, {"Все файлы", "*.*"}};
                options.fileName = "снимок.png";
                options.defaultExtension = "png";
                // Сохранение пишет настоящий текстовый файл, поэтому у него свои фильтры
                // The save writes a real text file, so it has its own filters
                if (kind == 1)
                    options = saveSampleOptions();
                if (kind == 0)
                    paths = application->openFileDialog(options, true, &node);
                else if (kind == 1)
                    paths.push_back(application->saveFileDialog(options, &node));
                else
                    paths.push_back(application->folderDialog(options, &node));
                if (paths.empty() || paths.front().empty())
                    setStatus(*application, "Отменено");
                else if (kind == 1)
                    setStatus(*application, saveSample(paths.front()));
                else
                    setStatus(*application, "Выбрано: " + paths.front() + " (всего " + std::to_string(paths.size()) + ")");
            }
        };

        // Второе окно: open — открыть tool_window.xml рядом с разметкой, иначе — закрыть окно узла
        // The second window: open — open tool_window.xml beside the markup, otherwise — close the node window
        struct WindowAction
        {
            gran::Application* application;
            std::string toolMarkup;
            bool open;

            void operator()(gran::ui::Node& node) const
            {
                if (open)
                {
                    application->openWindow(toolMarkup);
                    setStatus(*application, "Открыто второе окно");
                    return;
                }
                setStatus(*application, "Второе окно закрыто");
                application->closeWindow(node);
            }
        };

        // Брошенные файлы: число и первый путь — в зону сброса / The dropped files: the count and the first path go into the drop zone
        struct DropAction
        {
            gran::Application* application;

            void operator()(gran::ui::Node& zone, const std::vector<std::string>& files) const
            {
                gran::ui::Node* text = application->find("dropText");

                setStatus(*application, "Брошено файлов: " + std::to_string(files.size()));
                if (text == nullptr)
                    return;
                text->content.text = files.front();
                gran::ui::markDirty(*text, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
                (void)zone;
            }
        };
    }

    // Действия samples/platform.xml и samples/tool_window.xml / The samples/platform.xml and samples/tool_window.xml actions
    void registerPlatform(gran::Application& application, const std::string& markup)
    {
        std::string tool = (std::filesystem::path(markup).parent_path() / "tool_window.xml").string();

        application.onAction("openFile", FileAction{&application, 0});
        application.onAction("saveFile", FileAction{&application, 1});
        application.onAction("chooseFolder", FileAction{&application, 2});
        application.onAction("newWindow", WindowAction{&application, tool, true});
        application.onAction("closeTool", WindowAction{&application, tool, false});
        application.onAction("toolClosing", WindowAction{&application, tool, false});
        application.onFileDrop("filesDropped", DropAction{&application});
    }
}
