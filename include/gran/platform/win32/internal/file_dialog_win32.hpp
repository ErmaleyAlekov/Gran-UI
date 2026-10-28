#ifndef GRAN_PLATFORM_WIN32_INTERNAL_FILE_DIALOG_WIN32_HPP
#define GRAN_PLATFORM_WIN32_INTERNAL_FILE_DIALOG_WIN32_HPP

#include "gran/platform/file_dialog.hpp"
#include "gran/platform/win32/win32_window.hpp"

#include <shobjidl.h>

#include <string>

namespace gran::platform::win32
{
    // ---- win32_file_dialog_setup.cpp ----

    // Ошибка вызова COM диалога → GranException(PLATFORM) с кодом / A dialog COM call error → GranException(PLATFORM) with the code
    void checkDialog(HRESULT result);

    // Заголовок, типы файлов, папка, имя, расширение и флаги вида диалога / The title, the file types, the folder, the name, the extension and the dialog kind flags
    void applyDialogOptions(IFileDialog& dialog, FileDialogKind kind, const FileDialogOptions& options);

    // Путь файловой системы элемента оболочки в UTF-8 / The file system path of a shell item in UTF-8
    std::string shellItemPath(IShellItem& item);
}

#endif
