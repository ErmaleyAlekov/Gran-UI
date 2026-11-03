#include "gran/log/crash_report.hpp"
#include "gran/log/log.hpp"

#include <filesystem>
#include <fstream>

namespace gran::log
{
    namespace
    {
        // Самый новый отчёт .txt каталога (пустой путь — отчётов нет) / The newest .txt report of the directory (an empty path — no reports)
        std::filesystem::path newestReport(const std::filesystem::path& folder)
        {
            std::error_code error;
            std::filesystem::path newest;
            std::filesystem::file_time_type newestTime{};

            for (std::filesystem::directory_iterator entry(folder, error), end; !error && entry != end;
                entry.increment(error))
            {
                if (!entry->is_regular_file(error) || entry->path().extension() != ".txt")
                    continue;
                std::filesystem::file_time_type time = entry->last_write_time(error);
                if (newest.empty() || time > newestTime)
                {
                    newest = entry->path();
                    newestTime = time;
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
        return (std::filesystem::u8path(defaultDirectory(options.appName)).parent_path() / "crashes").u8string();
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
        std::filesystem::path folder = std::filesystem::u8path(directory);
        std::filesystem::path newest = newestReport(folder);
        std::string seen;

        if (newest.empty())
            return std::string();
        std::ifstream input(folder / ".seen");
        std::getline(input, seen);
        if (seen == newest.filename().u8string())
            return std::string();
        std::ofstream output(folder / ".seen", std::ios::trunc);
        output << newest.filename().u8string() << '\n';
        return newest.u8string();
    }
}
