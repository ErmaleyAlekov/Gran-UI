#include "gran/app/internal/app_state.hpp"
#include "gran/core/i18n.hpp"
#include "gran/log/crash_report.hpp"
#include "gran/log/log.hpp"

#include <filesystem>

namespace gran
{
    namespace app
    {
        /*
        ** Без имени программы журнал не открывается (тесты и примеры не мусорят в профиле пользователя).
        ** Отчёты о сбое — рядом с журналом: в своём каталоге журнала — его подкаталог crashes.
        ** Отчёт прошлого запуска запоминается сразу: после установки обработчика новых ещё нет.
        **
        ** Without a program name the log does not open (tests and samples do not litter the user profile).
        ** The crash reports go beside the log: with a custom log directory — its crashes subdirectory.
        ** The previous run report is remembered at once: right after installing the handler there are no new ones yet.
        */
        void startLogging(Application::State& state)
        {
            const AppOptions& options = state.options;
            log::LogOptions settings;
            log::CrashOptions crash;

            if (options.appName.empty())
                return;
            settings.appName = options.appName;
            settings.directory = options.logDirectory;
            log::open(settings);
            log::write(log::Level::INFO, "app", i18n::format(i18n::Message::LOG_START, {log::systemDescription()}));
            if (!options.crashReports)
                return;
            crash.appName = options.appName;
            if (!options.logDirectory.empty())
                crash.directory = (std::filesystem::u8path(options.logDirectory) / "crashes").u8string();
            log::installCrashHandler(crash);
            state.previousCrash = log::previousCrash(log::crashDirectory(crash));
            if (!state.previousCrash.empty())
                log::write(log::Level::WARNING, "app", i18n::format(i18n::Message::LOG_PREVIOUS_CRASH, {state.previousCrash}));
        }

        // «Direct3D 12 · AMD Radeon RX 9070 · 32.0.21025.1024»; программный адаптер помечается / A software adapter is marked
        void logAdapter(const AppWindow& win)
        {
            rhi::AdapterInfo info;
            std::string text;

            if (!win.backend)
                return;
            info = win.backend->adapterInfo();
            text = info.api + " · " + info.name + " · " + info.driver;
            if (info.software)
                text += " (software)";
            log::write(log::Level::INFO, "gpu", text);
        }
    }

    // Путь, найденный при запуске / The path found on start
    const std::string& Application::previousCrash() const
    {
        return _state->previousCrash;
    }
}
