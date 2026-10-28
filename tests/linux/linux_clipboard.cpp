#include "linux_tests.hpp"
#include <X11/Xlib.h>
#include <chrono>
#include <csignal>
#include <spawn.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

extern char** environ;

namespace linux_tests
{
    // Новый процесс безопасен и при работающих потоках Mesa, в отличие от fork с вызовами Xlib.
    // A new process is safe with Mesa worker threads running, unlike fork followed by Xlib calls.
    void clipboard(gran::platform::Window& window)
    {
        char program[] = "/proc/self/exe"; char option[] = "--clipboard-peer";
        char* arguments[] = {program, option, nullptr};
        pid_t child = -1;
        require(posix_spawn(&child, program, nullptr, nullptr, arguments, environ) == 0, "clipboard peer spawn");
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        int status = 0; pid_t result = 0;
        while (std::chrono::steady_clock::now() < deadline)
        {
            gran::platform::Event event;
            while (window.pollEvent(event)) {}
            result = waitpid(child, &status, WNOHANG);
            if (result != 0) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        if (result == 0) { kill(child, SIGKILL); waitpid(child, &status, 0); }
        require(result == child && WIFEXITED(status) && WEXITSTATUS(status) == 0, "external clipboard UTF-8");
    }

    // Клиент использует публичный Window API и собственное соединение с X-сервером.
    // The client uses the public Window API and its own X-server connection.
    int clipboardPeer()
    {
        gran::platform::WindowDesc desc;
        auto peer = gran::platform::createWindow(desc);
        require(peer->clipboardText() == "Берёста: UTF-8", "clipboard peer text");
        return 0;
    }
}
