#ifndef GRAN_PLATFORM_FILE_DIALOG_HPP
#define GRAN_PLATFORM_FILE_DIALOG_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace gran::platform
{
    class Window;

    // Тип файлов в диалоге: подпись и шаблоны через «;» («Картинки», «*.png;*.jpg») / A file type in a dialog: the label and the patterns separated by ";"
    struct FileFilter
    {
        std::string name;
        std::string patterns;
    };

    // Вид диалога / The dialog kind
    enum class FileDialogKind : std::uint8_t { OPEN, OPEN_MULTIPLE, SAVE, FOLDER };

    /*
    ** Параметры диалога (пустое поле — как решит ОС): заголовок, начальная папка, предлагаемое
    ** имя файла (сохранение), типы файлов (первый выбран), расширение по умолчанию без точки
    ** (сохранение: добавляется, если пользователь его не написал).
    **
    ** The dialog parameters (an empty field — as the OS decides): the title, the initial folder, the suggested
    ** file name (saving), the file types (the first one is selected), the default extension without a dot
    ** (saving: it is added if the user did not type it).
    */
    struct FileDialogOptions
    {
        std::string title;
        std::string folder;
        std::string fileName;
        std::vector<FileFilter> filters;
        std::string defaultExtension;
    };

    /*
    ** Модальный системный диалог над окном owner (nullptr — без владельца). Возвращает выбранные
    ** пути в UTF-8; пусто — пользователь отказался. Ошибка ОС — GranException(PLATFORM).
    ** Только поток интерфейса: пока диалог открыт, окна приложения копят события, а не рисуют.
    **
    ** A modal system dialog over the owner window (nullptr — no owner). Returns the chosen
    ** paths in UTF-8; empty — the user cancelled. An OS error is GranException(PLATFORM).
    ** The UI thread only: while the dialog is open, the application windows accumulate events instead of drawing.
    */
    std::vector<std::string> showFileDialog(FileDialogKind kind, const FileDialogOptions& options, Window* owner);
}

#endif
