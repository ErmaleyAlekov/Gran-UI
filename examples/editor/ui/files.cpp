#include "ui/controller.hpp"
#include "../core/messages.hpp"
#include "../core/text.hpp"
#include <filesystem>
namespace editor
{
    // Повторное открытие выбирает существующий буфер, не читая файл заново.
    // Reopening selects the existing buffer without reading the file again.
    void Controller::openDocument(const std::string& path)
    {
        if (pending)
            return;
        auto canonical = canonicalPath(path);
        for (const auto& item : documents)
        {
            std::error_code error;
            if (item.path == canonical || (!item.path.empty() &&
                std::filesystem::equivalent(std::filesystem::u8path(item.path), std::filesystem::u8path(canonical), error)))
            {
                select(item.id);
                return;
            }
        }
        std::string text;
        if (!readDocument(canonical, text))
            setText("statusMessage", message(Message::OPEN_FAILED));
        else
            createDocument(canonical, text);
    }

    // Отмена системного диалога не меняет открытые документы.
    // Cancelling the system dialog leaves open documents unchanged.
    void Controller::openDialog()
    {
        auto paths = app.openFileDialog(gran::FileDialogOptions{}, true);
        for (const auto& path : paths)
            openDocument(path);
    }

    // Сохранение отклоняется, если файл изменён другим приложением.
    // Saving is rejected when another application has changed the file.
    bool Controller::saveDocument(unsigned id, bool saveAs)
    {
        auto* item = document(id);
        if (!item) return false;
        std::string target = savePath(*item, saveAs);
        if (target.empty()) return false;
        target = canonicalPath(target);
        if (!canSaveTo(*item, target)) return false;
        if (!writeDocument(target, item->page->content.text))
        {
            setText("statusMessage", message(Message::SAVE_FAILED));
            return false;
        }
        item->path = canonicalPath(target);
        item->name = fileLabel(target);
        item->saved = item->page->content.text;
        refresh();
        setText("statusMessage", message(Message::SAVED) + target);
        return true;
    }

    // Не допускаем двух независимых буферов одного пути после Save As.
    // Save As must not create two independent buffers for the same path.
    bool Controller::canSaveTo(const Document& item, const std::string& target)
    {
        for (const auto& other : documents)
        {
            std::error_code error;
            bool same = other.path == target;
            if (!other.path.empty()) same = same || std::filesystem::equivalent(
                std::filesystem::u8path(other.path), std::filesystem::u8path(target), error);
            if (other.id != item.id && same)
            {
                setText("statusMessage", message(Message::SAVE_FAILED));
                return false;
            }
        }
        std::string disk;
        if (target == item.path && (!readDocument(target, disk) || disk != item.saved))
        {
            setText("statusMessage", message(Message::CHANGED_ON_DISK));
            return false;
        }
        return true;
    }

    // Обычное сохранение использует прежний путь, Save As запрашивает новый.
    // Normal saving reuses the existing path; Save As asks for a new one.
    std::string Controller::savePath(const Document& item, bool saveAs)
    {
        if (!saveAs && !item.path.empty()) return item.path;
        gran::FileDialogOptions options;
        options.fileName = item.name;
        return app.saveFileDialog(options, item.page);
    }
}
