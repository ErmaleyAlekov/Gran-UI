#include "gran/log/internal/log_state.hpp"

#include <algorithm>
#include <string>

namespace gran::log::internal
{
    namespace
    {
        // Файл номер index: 0 — сам журнал, иначе «имя.index.log» / The file number index: 0 — the log itself, otherwise "name.index.log"
        std::filesystem::path numberedPath(const LogState& state, unsigned index)
        {
            if (index == 0)
                return state.path;
            return state.folder / std::filesystem::u8path(state.options.appName + "." + std::to_string(index) + ".log");
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
        std::error_code error;

        state.file.close();
        if (files > 1)
        {
            std::filesystem::remove(numberedPath(state, files - 1), error);
            for (unsigned index = files - 1; index > 1; --index)
                std::filesystem::rename(numberedPath(state, index - 1), numberedPath(state, index), error);
            std::filesystem::rename(state.path, numberedPath(state, 1), error);
        }
        state.file.open(state.path, std::ios::out | std::ios::trunc | std::ios::binary);
        state.size = 0;
        state.open = static_cast<bool>(state.file);
    }

    // Дозапись в конец; слишком большой старый файл сразу уходит в .1.log / Appending at the end; a too large old file goes into .1.log at once
    bool openFile(LogState& state)
    {
        std::error_code error;

        state.file.open(state.path, std::ios::out | std::ios::app | std::ios::binary);
        if (!state.file)
            return false;
        state.size = std::filesystem::file_size(state.path, error);
        if (error)
            state.size = 0;
        state.open = true;
        if (state.size >= state.options.maxBytes)
            rotate(state);
        return state.open;
    }
}
