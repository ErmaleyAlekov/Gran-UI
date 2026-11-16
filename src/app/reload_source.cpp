#include "gran/app/internal/app_state.hpp"
#include "gran/utils/file_system.hpp"

#include <algorithm>

namespace gran::app
{
    // Исчезнувший на время сохранения редактором файл — не изменение (queryPath без исключений).
    // A file that vanished while an editor saves it is not a change (queryPath has no exceptions).
    // Файлы библиотек проверяются всегда: их отметки обновляются вместе с отметкой главного файла.
    // The library files are always checked: their stamps are updated together with the main file stamp.
    bool sourceChanged(Application::State& state)
    {
        bool libraries = librariesChanged(state);
        fs::FileInfo info;

        if (state.sourcePath.empty())
            return false;
        info = fs::queryPath(state.sourcePath);
        if (!info.exists || (info.modified == state.sourceTime && info.size == state.sourceSize))
            return libraries;
        state.sourceTime = info.modified;
        state.sourceSize = info.size;
        return true;
    }

    // Строковая разметка отключает наблюдение за файлом.
    // String markup disables file watching.
    void rememberSource(Application::State& state, const std::string& path)
    {
        state.sourcePath = path;
        state.sourceTime = 0;
        state.sourceSize = 0;
        sourceChanged(state);
    }

    // Строковая разметка берёт пути относительно текущего каталога.
    // String markup takes paths relative to the current directory.
    std::string sourceBase(const std::string& path)
    {
        if (path.empty())
            return ".";
        return fs::parentPath(path);
    }

    // Ноль — пора проверять файл; отрицательное — наблюдение выключено или файла нет.
    // Zero — time to check the file; negative — watching is off or there is no file.
    double reloadWait(const Application::State& state, double time) noexcept
    {
        if (!state.options.watchMarkup || state.sourcePath.empty())
            return -1.0;
        return std::max(0.0, state.nextReloadCheck - time);
    }
}
