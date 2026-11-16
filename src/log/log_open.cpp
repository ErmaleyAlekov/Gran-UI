#include "gran/log/internal/log_state.hpp"
#include "gran/utils/file_system.hpp"

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
        std::string folder = options.directory;

        if (options.appName.empty())
            return false;
        if (folder.empty())
            folder = defaultDirectory(options.appName);
        std::lock_guard<std::mutex> lock(state.mutex);
        std::string path = fs::joinPath(folder, options.appName + ".log");
        if (state.open && state.path == path)
            return true;
        state.file.close();
        state.open = false;
        state.options = options;
        state.folder = folder;
        state.path = path;
        fs::createDirectories(folder);
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
        return state.path;
    }

    // Каталог открытого журнала / The directory of the open log
    std::string directory()
    {
        internal::LogState& state = internal::logState();
        std::lock_guard<std::mutex> lock(state.mutex);

        if (!state.open)
            return std::string();
        return state.folder;
    }
}
