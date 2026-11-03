#include "gran/log/internal/log_state.hpp"

namespace gran::log
{
    namespace internal
    {
        // Состояние создаётся при первом обращении и живёт до выхода из процесса / The state is created on the first access and lives until the process exit
        LogState& logState() noexcept
        {
            static LogState state;

            return state;
        }
    }

    /*
    ** Каталог — свой или по умолчанию, создаётся со всеми родителями. Пустое имя программы журнала
    ** не открывает (файл назывался бы «.log»).
    **
    ** The directory is a custom or the default one, it is created with all the parents. An empty program name does not
    ** open the log (the file would be called ".log").
    */
    bool open(const LogOptions& options)
    {
        internal::LogState& state = internal::logState();
        std::filesystem::path folder;
        std::error_code error;

        if (options.appName.empty())
            return false;
        if (options.directory.empty())
            folder = std::filesystem::u8path(defaultDirectory(options.appName));
        else
            folder = std::filesystem::u8path(options.directory);
        std::lock_guard<std::mutex> lock(state.mutex);
        std::filesystem::path path = folder / std::filesystem::u8path(options.appName + ".log");
        if (state.open && state.path == path)
            return true;
        state.file.close();
        state.open = false;
        state.options = options;
        state.folder = folder;
        state.path = path;
        std::filesystem::create_directories(folder, error);
        return internal::openFile(state);
    }

    // Кольцо остаётся: отчёт о сбое после close всё ещё увидит последние записи / The ring stays: a crash report after close still sees the latest records
    void close() noexcept
    {
        internal::LogState& state = internal::logState();
        std::lock_guard<std::mutex> lock(state.mutex);

        state.file.close();
        state.open = false;
    }

    // Путь в UTF-8 (u8string), а не в кодовой странице ANSI / The path in UTF-8 (u8string), not in the ANSI code page
    std::string filePath()
    {
        internal::LogState& state = internal::logState();
        std::lock_guard<std::mutex> lock(state.mutex);

        if (!state.open)
            return std::string();
        return state.path.u8string();
    }

    // Каталог открытого журнала / The directory of the open log
    std::string directory()
    {
        internal::LogState& state = internal::logState();
        std::lock_guard<std::mutex> lock(state.mutex);

        if (!state.open)
            return std::string();
        return state.folder.u8string();
    }
}
