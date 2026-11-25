#include "gran/app/internal/app_state.hpp"
#include "gran/markup/components.hpp"
#include "gran/utils/file_system.hpp"

namespace gran::app
{
    namespace
    {
        // Файл, исчезнувший на время сохранения редактором, считается прежним / A file that vanished while an editor saves it counts as the former one
        bool stampChanged(FileStamp& stamp)
        {
            fs::FileInfo info = fs::queryPath(stamp.path);

            if (!info.exists || (info.modified == stamp.time && info.size == stamp.size))
                return false;
            stamp.time = info.modified;
            stamp.size = info.size;
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
    std::vector<FileStamp> libraryStamps(const markup::UiDocument& document)
    {
        std::vector<FileStamp> stamps;
        for (const std::string& path : markup::libraryFiles(document))
        {
            FileStamp& stamp = stamps.emplace_back(FileStamp{path, {}, 0});
            stampChanged(stamp);
        }
        return stamps;
    }
}
