#include "linux_tests.hpp"
#include "gran/log/log.hpp"
#include <chrono>
#include <csignal>
#include <spawn.h>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <vector>

extern char** environ;

namespace linux_tests
{
    namespace
    {
        // Что окно сообщило приложению за время броска / What the window told the application during the drop
        struct DropSeen
        {
            bool moved = false;
            bool dropped = false;
            bool left = false;
        };

        /*
        ** Запускает процесс-источник (--xdnd-peer) и крутит события окна, пока он не завершится: окно
        ** отвечает источнику только из своего цикла событий. true — источник вышел с кодом 0.
        **
        ** Starts the source process (--xdnd-peer) and pumps the window events until it ends: the window
        ** answers the source only from its event loop. true — the source exited with code 0.
        */
        bool runPeer(gran::platform::Window& window, std::vector<std::string> arguments, DropSeen& seen)
        {
            std::vector<char*> pointers;
            pid_t child = -1;
            int status = 0;
            pid_t result = 0;

            for (std::string& argument : arguments)
                pointers.push_back(argument.data());
            pointers.push_back(nullptr);
            require(posix_spawn(&child, "/proc/self/exe", nullptr, nullptr, pointers.data(), environ) == 0, "xdnd peer spawn");
            auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(12);
            while (result == 0 && std::chrono::steady_clock::now() < deadline)
            {
                gran::platform::Event event;
                while (window.pollEvent(event))
                {
                    seen.moved = seen.moved || event.kind == gran::platform::EventKind::DRAG_MOVE;
                    seen.dropped = seen.dropped || event.kind == gran::platform::EventKind::DROP;
                    seen.left = seen.left || event.kind == gran::platform::EventKind::DRAG_LEAVE;
                }
                result = waitpid(child, &status, WNOHANG);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            if (result == 0) { kill(child, SIGKILL); waitpid(child, &status, 0); }
            return result == child && WIFEXITED(status) && WEXITSTATUS(status) == 0;
        }

        // Один бросок в точку окна (логические пиксели); accept — ждём ли приёма / One drop at a window point (logical pixels); accept — whether acceptance is expected
        void dropAt(gran::platform::Window& window, gran::Vec2 point, bool accept, DropSeen& seen)
        {
            std::string target = std::to_string(reinterpret_cast<std::uintptr_t>(window.nativeHandle()));
            std::string x = std::to_string(static_cast<int>(point.x * window.scale()));
            std::string y = std::to_string(static_cast<int>(point.y * window.scale()));
            std::string expected = "0";

            if (accept)
                expected = "1";
            require(runPeer(window, {"/proc/self/exe", "--xdnd-peer", target, x, y, expected}, seen), "xdnd peer answers");
        }

        /*
        ** Журнал сеанса XDND (x11_drop_log.cpp) записан: в кольце есть начало с «text/uri-list: yes» и упоминания типа
        ** в списке типов и в строке данных (текст строк зависит от языка, имя типа — нет). По этим строкам разбирают
        ** живые файловые менеджеры (Т-38), поэтому их пропажа — ошибка.
        **
        ** The XDND session log (x11_drop_log.cpp) is written: the ring has the start with "text/uri-list: yes" and mentions
        ** of the type in the type list and in the data line (the line text depends on the language, the type name does not).
        ** Live file managers are analysed by these lines (T-38), so losing them is a bug.
        */
        bool dropLogged()
        {
            static char recent[16384];
            std::string text(recent, gran::log::copyRecent(recent, sizeof(recent)));
            std::size_t mentions = 0;

            for (std::size_t at = text.find("text/uri-list"); at != std::string::npos; at = text.find("text/uri-list", at + 1))
                ++mentions;
            return text.find("text/uri-list: yes") != std::string::npos && mentions >= 3;
        }
    }

    /*
    ** Перетаскивание файлов по XDND: бросок над зоной даёт DRAG_MOVE, DROP и пути (кириллица и пробел
    ** раскодированы, комментарий пропущен); бросок мимо зоны — отказ, DRAG_LEAVE и ни одного пути.
    **
    ** Dragging files by XDND: a drop over a zone gives DRAG_MOVE, DROP and the paths (Cyrillic and a space
    ** decoded, the comment skipped); a drop beside the zone is a refusal, DRAG_LEAVE and not a single path.
    */
    void drop(gran::platform::Window& window)
    {
        DropSeen inside;
        DropSeen outside;

        window.setDropZones({gran::Rect{0, 0, 40, 40}});
        dropAt(window, gran::Vec2{10, 10}, true, inside);
        require(inside.moved && inside.dropped, "xdnd drop events");
        std::vector<std::string> paths = window.takeDroppedFiles();
        require(paths.size() == 2 && paths[0] == "/tmp/п a.txt" && paths[1] == "/tmp/b.txt", "xdnd dropped paths");
        require(dropLogged(), "xdnd session log");
        dropAt(window, gran::Vec2{80, 60}, false, outside);
        require(outside.moved && outside.left && !outside.dropped, "xdnd refused drop events");
        require(window.takeDroppedFiles().empty(), "xdnd refused drop paths");
    }
}
