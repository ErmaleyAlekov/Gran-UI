#include "gran/platform/x11/x11_file_dialog.hpp"
#include "gran/platform/window.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <cstdint>
#include <filesystem>

namespace gran::platform::x11
{
    namespace
    {
        // Начальный путь: папка и предлагаемое имя; папке нужен «/», иначе её примут за имя файла
        // The start path: the folder and the suggested name; a folder needs "/", otherwise it is taken for a file name
        std::string startPath(const FileDialogOptions& options)
        {
            std::string folder = options.folder;

            if (folder.empty())
                folder = ".";
            if (folder.back() != '/')
                folder += '/';
            return folder + options.fileName;
        }

        // Шаблоны «*.png;*.jpg» → «*.png *.jpg»: обе программы разделяют их пробелом
        // The "*.png;*.jpg" patterns → "*.png *.jpg": both programs separate them with a space
        std::string spacedPatterns(std::string patterns)
        {
            for (char& symbol : patterns)
            {
                if (symbol == ';')
                    symbol = ' ';
            }
            return patterns;
        }
    }

    // Фильтры zenity: «Имя | *.png *.jpg»; --attach не передаётся — его нет в zenity 4
    // The zenity filters: "Name | *.png *.jpg"; --attach is not passed — zenity 4 does not have it
    std::vector<std::string> zenityArguments(FileDialogKind kind, const FileDialogOptions& options, unsigned long)
    {
        std::vector<std::string> arguments{"zenity", "--file-selection", "--filename=" + startPath(options)};

        if (!options.title.empty())
            arguments.push_back("--title=" + options.title);
        if (kind == FileDialogKind::OPEN_MULTIPLE)
            arguments.insert(arguments.end(), {"--multiple", "--separator=\n"});
        else if (kind == FileDialogKind::SAVE)
            arguments.insert(arguments.end(), {"--save", "--confirm-overwrite"});
        else if (kind == FileDialogKind::FOLDER)
            arguments.push_back("--directory");
        for (const FileFilter& filter : options.filters)
        {
            arguments.push_back("--file-filter=" + filter.name + " | " + spacedPatterns(filter.patterns));
        }
        return arguments;
    }

    // kdialog: вид — отдельной командой, фильтр — первый из списка в виде «Имя (*.png *.jpg)»
    // kdialog: the kind is a separate command, the filter is the first of the list as "Name (*.png *.jpg)"
    std::vector<std::string> kdialogArguments(FileDialogKind kind, const FileDialogOptions& options, unsigned long owner)
    {
        static const char* const COMMANDS[] = {"--getopenfilename", "--getopenfilename", "--getsavefilename", "--getexistingdirectory"};
        std::vector<std::string> arguments{"kdialog"};

        if (!options.title.empty())
            arguments.insert(arguments.end(), {"--title", options.title});
        if (owner != 0)
            arguments.insert(arguments.end(), {"--attach", std::to_string(owner)});
        if (kind == FileDialogKind::OPEN_MULTIPLE)
            arguments.insert(arguments.end(), {"--multiple", "--separate-output"});
        arguments.push_back(COMMANDS[static_cast<int>(kind)]);
        arguments.push_back(startPath(options));
        if (kind != FileDialogKind::FOLDER && !options.filters.empty())
            arguments.push_back(options.filters.front().name + " (" + spacedPatterns(options.filters.front().patterns) + ")");
        return arguments;
    }
}

namespace gran::platform
{
    // Сначала zenity, затем kdialog; нет ни одной — понятная ошибка, а не тихий «отказ пользователя»
    // zenity first, then kdialog; neither exists — a clear error rather than a silent "user refusal"
    std::vector<std::string> showFileDialog(FileDialogKind kind, const FileDialogOptions& options, Window* owner)
    {
        unsigned long id = 0;

        if (owner != nullptr)
            id = static_cast<unsigned long>(reinterpret_cast<std::uintptr_t>(owner->nativeHandle()));
        x11::DialogRun run = x11::runDialog(x11::zenityArguments(kind, options, id));
        if (!run.started)
            run = x11::runDialog(x11::kdialogArguments(kind, options, id));
        if (!run.started)
            throw GranException(ErrorCode::PLATFORM, i18n::format(i18n::Message::PLATFORM_FILE_DIALOG_TOOL));
        // Имя сохраняемого файла без расширения получает defaultExtension (как в диалоге Windows)
        // A saved file name without an extension gets defaultExtension (as in the Windows dialog)
        for (std::string& path : run.lines)
        {
            if (kind == FileDialogKind::SAVE && !options.defaultExtension.empty() && !std::filesystem::path(path).has_extension())
                path += "." + options.defaultExtension;
        }
        return run.lines;
    }
}
