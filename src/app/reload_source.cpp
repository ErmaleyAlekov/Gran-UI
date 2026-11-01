#include "gran/app/internal/app_state.hpp"

#include <algorithm>

namespace gran::app
{
    // error_code допускает временное исчезновение файла при сохранении редактором.
    // error_code allows temporary file disappearance while an editor saves it.
    // Файлы библиотек проверяются всегда: их отметки обновляются вместе с отметкой главного файла.
    // The library files are always checked: their stamps are updated together with the main file stamp.
    bool sourceChanged(Application::State& state)
    {
        std::error_code error;
        bool libraries = librariesChanged(state);

        if (state.sourcePath.empty())
            return false;
        auto time = std::filesystem::last_write_time(state.sourcePath, error);
        if (error)
            return libraries;
        auto size = std::filesystem::file_size(state.sourcePath, error);
        if (error || (time == state.sourceTime && size == state.sourceSize))
            return libraries;
        state.sourceTime = time;
        state.sourceSize = size;
        return true;
    }

    // Строковая разметка отключает наблюдение за файлом.
    // String markup disables file watching.
    void rememberSource(Application::State& state, const std::string& path)
    {
        state.sourcePath = path;
        state.sourceTime = std::filesystem::file_time_type{};
        state.sourceSize = 0;
        sourceChanged(state);
    }

    // Строковая разметка берёт пути относительно текущего каталога.
    // String markup takes paths relative to the current directory.
    std::string sourceBase(const std::string& path)
    {
        if (path.empty())
            return ".";
        return std::filesystem::path(path).parent_path().string();
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
