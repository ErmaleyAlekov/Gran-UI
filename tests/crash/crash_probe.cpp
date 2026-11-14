/*
** gran_crash_probe — проверка отчёта о сбое (make test-crash). Режим --check <каталог> запускает саму себя
** с --crash <вид> <каталог/вид> для каждого вида падения и проверяет отчёты: текст с заголовком и последней
** записью журнала; на Windows ещё минидамп с подписью MDMP, на Linux и macOS — backtrace, модули и файл .stack.
**
** gran_crash_probe — the crash report check (make test-crash). The --check <directory> mode runs itself
** with --crash <kind> <directory/kind> for every crash kind and checks the reports: a text with the header and the latest
** log record; on Windows also a minidump with the MDMP signature, on Linux and macOS — backtrace, the modules and the .stack file.
*/
#include "gran/log/crash_report.hpp"
#include "gran/log/log.hpp"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace
{
    // Виды падения / The crash kinds
    const char* const KINDS[] = {"segv", "terminate", "abort"};

    // Упасть заданным способом после записи журнала / Crash in the given way after a log record
    [[noreturn]] void crash(const std::string& kind, const std::string& folder)
    {
        gran::log::LogOptions options;
        gran::log::CrashOptions report;

        options.appName = "probe";
        options.directory = folder;
        report.appName = "probe";
        report.directory = folder;
        gran::log::open(options);
        gran::log::installCrashHandler(report);
        gran::log::write(gran::log::Level::INFO, "probe", "before crash " + kind);
        if (kind == "segv")
            *static_cast<volatile int*>(nullptr) = 1;
        if (kind == "terminate")
            throw std::runtime_error("probe exception");
        std::abort();
    }

    // Файл целиком (двоичный) / The whole file (binary)
    std::string readAll(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        std::ostringstream text;

        text << input.rdbuf();
        return text.str();
    }

    // Отчёт вида kind: текст, хвост журнала и сопутствующий файл ОС / The report of the kind: the text, the log tail and the OS companion file
    bool checkReport(const std::filesystem::path& folder, const std::string& kind)
    {
        bool text = false;
        bool companion = false;
        std::error_code error;

        for (std::filesystem::directory_iterator entry(folder, error), end; !error && entry != end; entry.increment(error))
        {
            std::string content = readAll(entry->path());
            if (entry->path().extension() == ".txt")
                text = content.find("Gran UI crash report") != std::string::npos
                    && content.find("before crash " + kind) != std::string::npos;
#if defined(_WIN32)
            if (entry->path().extension() == ".dmp")
                companion = content.compare(0, 4, "MDMP") == 0;
#else
            if (entry->path().extension() == ".stack")
                companion = !content.empty();
#endif
        }
        std::cout << kind << ": text " << text << ", companion " << companion << '\n';
        return text && companion;
    }

    // Запуск себя в режиме падения и проверка отчёта / Running itself in the crash mode and checking the report
    bool runKind(const std::string& program, const std::filesystem::path& base, const std::string& kind)
    {
        std::filesystem::path folder = base / kind;
        std::error_code error;
        std::string command = program + " --crash " + kind + " " + folder.string();

        std::filesystem::remove_all(folder, error);
        std::filesystem::create_directories(folder, error);
        // Код выхода упавшей программы ненулевой; важны файлы отчёта, а не код / The crashed program exit code is non-zero; the report files matter, not the code
        int status = std::system(command.c_str());
        std::cout << kind << ": exit status " << status << '\n';
        return checkReport(folder, kind);
    }
}

// --crash <вид> <каталог> — упасть; --check <каталог> — проверить все виды / --crash <kind> <directory> — crash; --check <directory> — check all the kinds
int main(int argc, char* argv[])
{
    bool ok = true;

    if (argc == 4 && std::strcmp(argv[1], "--crash") == 0)
        crash(argv[2], argv[3]);
    if (argc != 3 || std::strcmp(argv[1], "--check") != 0)
    {
        std::cerr << "usage: gran_crash_probe --check <directory>\n";
        return 2;
    }
    for (const char* kind : KINDS)
        ok = runKind(argv[0], argv[2], kind) && ok;
    if (!ok)
    {
        std::cout << "crash reports FAILED\n";
        return 1;
    }
    std::cout << "crash reports OK\n";
    return 0;
}
