#include "gran/platform/headless/headless_window.hpp"

namespace gran::platform
{
    namespace
    {
        /*
        ** Буфер обмена безоконных окон — один на процесс, как у ОС. Объект живёт в функции (создаётся при первом
        ** обращении, без проблемы порядка инициализации глобальных); мьютекс — тесты могут идти в разных потоках.
        **
        ** The clipboard of the windowless windows is one per process, as the OS one. The object lives in a function (created on
        ** the first access, without the global initialization order problem); the mutex — tests may run in different threads.
        */
        struct SharedClipboard
        {
            std::mutex mutex;
            std::string text;
        };

        SharedClipboard& clipboard()
        {
            static SharedClipboard shared;

            return shared;
        }
    }

    std::string HeadlessWindow::clipboardText()
    {
        std::lock_guard<std::mutex> lock(clipboard().mutex);

        return clipboard().text;
    }

    void HeadlessWindow::setClipboardText(const std::string& text)
    {
        std::lock_guard<std::mutex> lock(clipboard().mutex);

        clipboard().text = text;
    }

    void HeadlessWindow::setDropZones(const std::vector<Rect>& zones)
    {
        _state.dropZones = zones;
    }

    // Пути самого раннего броска (dropFiles кладёт их под мьютексом перед событием DROP)
    // The paths of the earliest drop (dropFiles puts them under the mutex before the DROP event)
    std::vector<std::string> HeadlessWindow::takeDroppedFiles()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        std::vector<std::string> paths;

        if (_drops.empty())
            return paths;
        paths = std::move(_drops.front());
        _drops.pop_front();
        return paths;
    }
}
