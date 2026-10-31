#include "ui/controller.hpp"
#include "../core/messages.hpp"
#include "../core/text.hpp"
#include <algorithm>
#include <filesystem>
namespace editor
{
    // Смена рабочей папки не закрывает существующие документы.
    // Changing the workspace folder does not close existing documents.
    void Controller::openFolder()
    {
        auto path = app.folderDialog(gran::FileDialogOptions{});
        if (!path.empty()) loadFolder(path);
    }

    // Корень проводника показывает файлы папки без рекурсивного обхода проекта.
    // The explorer root shows folder entries without recursively walking the project.
    void Controller::loadFolder(const std::string& path)
    {
        if (pending) return;
        std::error_code error;
        std::filesystem::directory_iterator entries(std::filesystem::u8path(path), error);
        if (error) { setText("statusMessage", message(Message::FOLDER_FAILED)); return; }
        workspace = path;
        auto* host = app.find("explorerFiles");
        while (!host->children.empty()) app.remove(*host->children.back());
        std::vector<std::string> paths;
        for (; entries != std::filesystem::directory_iterator(); entries.increment(error))
        {
            if (error || paths.size() >= 400) break;
            const auto& entry = *entries;
            if (entry.is_regular_file(error) || entry.is_directory(error)) paths.push_back(entry.path().u8string());
        }
        std::sort(paths.begin(), paths.end());
        if (std::filesystem::u8path(path).has_parent_path())
            addExplorerEntry(std::filesystem::u8path(path).parent_path().u8string());
        for (const auto& entry : paths) addExplorerEntry(entry);
        setText("workspaceName", fileLabel(path));
    }

    // Папка меняет текущий уровень; файл открывается во вкладке.
    // A folder changes the current level; a file opens in a tab.
    void Controller::addExplorerEntry(const std::string& path)
    {
        auto& node = add(*app.find("explorerFiles"), gran::ui::ElementKind::BUTTON, "explorer-file", fileLabel(path));
        node.content.option = path;
        node.content.tooltip = path;
        node.content.onClick = "openEntry";
        std::error_code error;
        if (std::filesystem::is_directory(std::filesystem::u8path(path), error))
        {
            node.content.text = "› " + node.content.text;
            node.content.onClick = "browseFolder";
        }
    }
}
