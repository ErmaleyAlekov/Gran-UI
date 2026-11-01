#include "gran/app/internal/app_state.hpp"
#include "gran/markup/components.hpp"

namespace gran::app
{
    namespace
    {
        // Файл, исчезнувший на время сохранения редактором, считается прежним / A file that vanished while an editor saves it counts as the former one
        bool stampChanged(FileStamp& stamp)
        {
            std::error_code error;
            auto time = std::filesystem::last_write_time(stamp.path, error);

            if (error)
                return false;
            auto size = std::filesystem::file_size(stamp.path, error);
            if (error || (time == stamp.time && size == stamp.size))
                return false;
            stamp.time = time;
            stamp.size = size;
            return true;
        }
    }

    bool librariesChanged(Application::State& state)
    {
        bool changed = false;

        for (FileStamp& stamp : state.libraryStamps)
        {
            if (stampChanged(stamp))
                changed = true;
        }
        return changed;
    }

    // Отметки сразу текущие: изменением считается только правка после загрузки / The stamps are current at once: only an edit after loading counts as a change
    void rememberLibraries(Application::State& state, const markup::UiDocument& document)
    {
        state.libraryStamps.clear();
        for (const std::string& path : markup::libraryFiles(document))
            state.libraryStamps.push_back(FileStamp{path, {}, 0});
        librariesChanged(state);
    }
}
