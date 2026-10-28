#include "gran/platform/x11/x11_file_dialog.hpp"

#include <cerrno>
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

namespace gran::platform::x11
{
    namespace
    {
        // Стандартный вывод ребёнка — в pipe; без fork, поэтому безопасно и при запущенных потоках драйвера.
        // The child standard output goes into a pipe; no fork, so it is safe with running driver threads too.
        bool spawnDialog(const std::vector<std::string>& arguments, pid_t& child, int& output)
        {
            std::vector<char*> argv;
            int channel[2];
            posix_spawn_file_actions_t actions;

            for (const std::string& argument : arguments)
                argv.push_back(const_cast<char*>(argument.c_str()));
            argv.push_back(nullptr);
            if (pipe2(channel, O_CLOEXEC) != 0)
                return false;
            posix_spawn_file_actions_init(&actions);
            posix_spawn_file_actions_adddup2(&actions, channel[1], STDOUT_FILENO);
            int result = posix_spawnp(&child, argv[0], &actions, nullptr, argv.data(), environ);
            posix_spawn_file_actions_destroy(&actions);
            ::close(channel[1]);
            output = channel[0];
            if (result == 0)
                return true;
            ::close(channel[0]);
            return false;
        }

        // Вывод читается целиком до EOF (EINTR повторяется) / The output is read entirely until EOF (EINTR is repeated)
        std::string readOutput(int output)
        {
            std::string text;
            char buffer[4096];
            ssize_t count = 0;

            while ((count = ::read(output, buffer, sizeof(buffer))) != 0)
            {
                if (count > 0)
                    text.append(buffer, static_cast<std::size_t>(count));
                else if (errno != EINTR)
                    break;
            }
            ::close(output);
            return text;
        }

        // Пути по строкам; пустые строки (хвостовой перевод строки) пропускаются / The paths by lines; empty lines (the trailing newline) are skipped
        std::vector<std::string> splitLines(const std::string& text)
        {
            std::vector<std::string> lines;
            std::size_t start = 0;

            while (start < text.size())
            {
                std::size_t end = text.find('\n', start);
                if (end == std::string::npos)
                    end = text.size();
                if (end > start)
                    lines.push_back(text.substr(start, end - start));
                start = end + 1;
            }
            return lines;
        }
    }

    // Код выхода не 0 — отказ (zenity и kdialog возвращают 1 на «Отмена») / A non-zero exit code is a refusal (zenity and kdialog return 1 on "Cancel")
    DialogRun runDialog(const std::vector<std::string>& arguments)
    {
        DialogRun run;
        pid_t child = 0;
        int output = -1;
        int status = 0;

        if (!spawnDialog(arguments, child, output))
            return run;
        std::string text = readOutput(output);
        while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}
        run.started = !(WIFEXITED(status) && WEXITSTATUS(status) == 127);
        if (run.started && WIFEXITED(status) && WEXITSTATUS(status) == 0)
            run.lines = splitLines(text);
        return run;
    }
}
