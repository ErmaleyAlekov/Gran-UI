#include "gran/log/crash_report.hpp"
#include "gran/log/log.hpp"

#include "gran/utils/file_system.hpp"

#include <fstream>

namespace gran::log
{
    namespace
    {
        // Самый новый отчёт .txt каталога (пустой путь — отчётов нет) / The newest .txt report of the directory (an empty path — no reports)
        std::string newestReport(const std::string& folder)
        {
            std::string newest;
            std::int64_t newestTime = 0;

            for (const fs::DirectoryEntry& entry : fs::listDirectory(folder))
            {
                if (!entry.info.regular || fs::fileExtension(entry.name) != ".txt")
                    continue;
                if (newest.empty() || entry.info.modified > newestTime)
                {
                    newest = entry.name;
                    newestTime = entry.info.modified;
                }
            }
            return newest;
        }
    }

    // Свой каталог или «crashes» рядом с «logs» каталога программы по умолчанию / A custom directory or "crashes" beside "logs" of the default program directory
    std::string crashDirectory(const CrashOptions& options)
    {
        if (!options.directory.empty())
            return options.directory;
        return fs::joinPath(fs::parentPath(defaultDirectory(options.appName)), "crashes");
    }

    /*
    ** Имя последнего показанного отчёта хранится в «.seen» того же каталога. Новый отчёт отличается
    ** именем (в нём время и номер процесса), поэтому сравнения имён достаточно.
    **
    ** The name of the last shown report is kept in ".seen" of the same directory. A new report differs
    ** by its name (it has the time and the process number), so comparing the names is enough.
    */
    std::string previousCrash(const std::string& directory)
    {
        std::string newest = newestReport(directory);
        std::string seenPath = fs::joinPath(directory, ".seen");
        std::string seen;
        std::ifstream input;
        std::ofstream output;

        if (newest.empty())
            return std::string();
        fs::openFile(input, seenPath, std::ios::in);
        std::getline(input, seen);
        if (seen == newest)
            return std::string();
        fs::openFile(output, seenPath, std::ios::trunc);
        output << newest << '\n';
        return fs::joinPath(directory, newest);
    }
}
