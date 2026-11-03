#include "gran/capi/internal/bridge.hpp"
#include "gran/log/crash_report.hpp"
#include "gran/log/log.hpp"

#include <filesystem>
#include <mutex>
#include <string>

namespace
{
    // Отчёт прошлого запуска, найденный gran_log_open (на весь процесс, под замком) / The previous run report found by gran_log_open (process-wide, under the lock)
    std::string& previousReport(std::unique_lock<std::mutex>& lock)
    {
        static std::mutex mutex;
        static std::string report;

        lock = std::unique_lock<std::mutex>(mutex);
        return report;
    }
}

// Каталог отчётов — crashes рядом со своим каталогом журнала или по умолчанию / The report directory is crashes beside the custom log directory or the default one
gran_status gran_log_open(const char* app_name, const char* directory, int32_t crash_reports)
{
    GRAN_CAPI_TRY
    gran::capi::require(app_name != nullptr && app_name[0] != '\0');
    gran::log::LogOptions options;
    gran::log::CrashOptions crash;
    options.appName = app_name;
    options.directory = gran::capi::text(directory);
    gran::capi::require(gran::log::open(options));
    if (crash_reports == 0)
        return GRAN_OK;
    crash.appName = app_name;
    if (directory != nullptr)
        crash.directory = (std::filesystem::u8path(directory) / "crashes").u8string();
    gran::log::installCrashHandler(crash);
    std::unique_lock<std::mutex> lock;
    previousReport(lock) = gran::log::previousCrash(gran::log::crashDirectory(crash));
    GRAN_CAPI_END
}

// Неизвестный уровень — неверный аргумент / An unknown level is an invalid argument
gran_status gran_log_write(int32_t level, const char* category, const char* text)
{
    GRAN_CAPI_TRY
    gran::capi::require(level >= GRAN_LOG_SEVERE && level <= GRAN_LOG_VERBOSE);
    gran::log::write(static_cast<gran::log::Level>(level), gran::capi::text(category), gran::capi::text(text));
    GRAN_CAPI_END
}

// Путь открытого журнала ("" — не открыт) / The open log path ("" — not open)
gran_status gran_log_path(char** result)
{
    if (result != nullptr) *result = nullptr;
    GRAN_CAPI_TRY
    gran::capi::require(result != nullptr);
    *result = gran::capi::copyString(gran::log::filePath());
    GRAN_CAPI_END
}

// Отчёт прошлого запуска ("" — его нет) / The previous run report ("" — there is none)
gran_status gran_log_previous_crash(char** result)
{
    if (result != nullptr) *result = nullptr;
    GRAN_CAPI_TRY
    gran::capi::require(result != nullptr);
    std::unique_lock<std::mutex> lock;
    *result = gran::capi::copyString(previousReport(lock));
    GRAN_CAPI_END
}
