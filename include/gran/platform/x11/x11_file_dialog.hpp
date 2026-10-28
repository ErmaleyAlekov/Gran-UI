#ifndef GRAN_PLATFORM_X11_FILE_DIALOG_HPP
#define GRAN_PLATFORM_X11_FILE_DIALOG_HPP

#include "gran/platform/file_dialog.hpp"

#include <string>
#include <vector>

namespace gran::platform::x11
{
    /*
    ** Диалоги файлов Linux без новых библиотек: системная программа zenity (GTK) или kdialog (KDE, Fly)
    ** запускается отдельным процессом, выбор читается из её стандартного вывода построчно.
    **
    ** Linux file dialogs without new libraries: the system zenity (GTK) or kdialog (KDE, Fly) program
    ** runs as a separate process, the choice is read from its standard output line by line.
    */

    // Итог запуска программы диалога / The result of running a dialog program
    struct DialogRun
    {
        // Программа найдена и запущена / The program was found and started
        bool started = false;
        // Строки вывода (пути) при коде выхода 0; пусто — пользователь отказался / Output lines (paths) on exit code 0; empty — the user refused
        std::vector<std::string> lines;
    };

    // ---- x11_file_dialog.cpp ----

    // Аргументы zenity и kdialog; owner — идентификатор окна X (0 — без владельца) / The zenity and kdialog arguments; owner — the X window id (0 — no owner)
    std::vector<std::string> zenityArguments(FileDialogKind kind, const FileDialogOptions& options, unsigned long owner);
    std::vector<std::string> kdialogArguments(FileDialogKind kind, const FileDialogOptions& options, unsigned long owner);

    // ---- x11_file_dialog_run.cpp ----

    // Запуск программы (поиск по PATH) и ожидание её завершения / Starting the program (a PATH lookup) and waiting for it to finish
    DialogRun runDialog(const std::vector<std::string>& arguments);
}

#endif
