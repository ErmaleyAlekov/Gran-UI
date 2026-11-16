#include "gran/log/internal/log_state.hpp"
#include "gran/utils/file_system.hpp"

#include <algorithm>
#include <string>

namespace gran::log::internal
{
    namespace
    {
        // Файл номер index: 0 — сам журнал, иначе «имя.index.log» / The file number index: 0 — the log itself, otherwise "name.index.log"
        std::string numberedPath(const LogState& state, unsigned index)
        {
            if (index == 0)
                return state.path;
            return fs::joinPath(state.folder, state.options.appName + "." + std::to_string(index) + ".log");
        }
    }

    /*
    ** Самый старый файл удаляется, остальные сдвигаются на номер вверх, текущий становится .1.log.
    ** Ошибки файловой системы (файл занят антивирусом) не останавливают журнал: пишем дальше в новый файл.
    ** При files == 1 журнал просто начинается заново.
    **
    ** The oldest file is removed, the others shift one number up, the current one becomes .1.log.
    ** File system errors (a file held by an antivirus) do not stop the log: we keep writing into a new file.
    ** With files == 1 the log simply starts anew.
    */
    void rotate(LogState& state)
    {
        unsigned files = std::max(1u, state.options.files);

        state.file.close();
        if (files > 1)
        {
            fs::removePath(numberedPath(state, files - 1));
            for (unsigned index = files - 1; index > 1; --index)
                fs::renamePath(numberedPath(state, index - 1), numberedPath(state, index));
            fs::renamePath(state.path, numberedPath(state, 1));
        }
        state.file.clear();
        fs::openFile(state.file, state.path, std::ios::trunc | std::ios::binary);
        state.size = 0;
        state.open = static_cast<bool>(state.file);
    }

    // Дозапись в конец; слишком большой старый файл сразу уходит в .1.log / Appending at the end; a too large old file goes into .1.log at once
    bool openFile(LogState& state)
    {
        state.file.clear();
        fs::openFile(state.file, state.path, std::ios::app | std::ios::binary);
        if (!state.file)
            return false;
        state.size = fs::queryPath(state.path).size;
        state.open = true;
        if (state.size >= state.options.maxBytes)
            rotate(state);
        return state.open;
    }
}
