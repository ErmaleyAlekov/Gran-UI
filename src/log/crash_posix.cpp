#if !defined(_WIN32)

#include "gran/log/crash_report.hpp"
#include "gran/log/internal/crash_posix.hpp"

#include <atomic>
#include <time.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <execinfo.h>

#include <mutex>

#include "gran/utils/file_system.hpp"

namespace gran::log
{
    namespace internal
    {
        // Состояние живёт до конца процесса / The state lives until the process end
        PosixCrash& posixCrash() noexcept
        {
            static PosixCrash state;

            return state;
        }
    }

    namespace
    {
        // Сигналы падения / The crash signals
        const int SIGNALS[] = {SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT};

        // Один поток пишет отчёт, остальные ждут до 100 мс / One writer, other threads wait up to 100 ms
        std::atomic<unsigned> reportPhase{0};
        static_assert(std::atomic<unsigned>::is_always_lock_free);
        thread_local char terminateMessage[512]{};

        void onSignal(int signal, siginfo_t* info, void* context)
        {
            unsigned idle = 0;
            if (reportPhase.compare_exchange_strong(idle, 1))
            {
                internal::writeSignalReport(signal, info, context, terminateMessage);
                reportPhase.store(2);
            }
            else
            {
                const timespec interval{0, 1000000};
                for (unsigned i = 0; i < 100 && reportPhase.load() == 1; ++i)
                    nanosleep(&interval, nullptr);
            }
            struct sigaction action{};
            action.sa_handler = SIG_DFL;
            sigemptyset(&action.sa_mask);
            sigaction(signal, &action, nullptr);
            std::raise(signal);
        }

        // std::terminate: текст непойманного исключения — в отчёт, затем abort (→ SIGABRT) / std::terminate: the uncaught exception text goes into the report, then abort (→ SIGABRT)
        void onTerminate()
        {
            std::exception_ptr current = std::current_exception();

            std::snprintf(terminateMessage, sizeof(terminateMessage), "std::terminate");
            try
            {
                if (current)
                    std::rethrow_exception(current);
            }
            catch (const std::exception& error)
            {
                std::snprintf(terminateMessage, sizeof(terminateMessage), "std::terminate: %s", error.what());
            }
            catch (...)
            {
            }
            std::abort();
        }

        /*
        ** Свой стек сигналов и обработчики. Первый вызов backtrace подгружает libgcc заранее:
        ** в обработчике сигнала загрузка библиотеки (с выделением памяти) недопустима.
        **
        ** An own signal stack and the handlers. The first backtrace call loads libgcc in advance:
        ** loading a library (with memory allocation) is not allowed in a signal handler.
        */
        bool installSignals() noexcept
        {
            static char signalStack[64 * 1024];
            stack_t stack{};
            struct sigaction action{};
            void* frames[2];

            stack.ss_sp = signalStack;
            stack.ss_size = sizeof(signalStack);
            if (sigaltstack(&stack, nullptr) != 0)
                return false;
            backtrace(frames, 2);
            action.sa_sigaction = onSignal;
            action.sa_flags = SA_SIGINFO | SA_ONSTACK;
            sigemptyset(&action.sa_mask);
            for (int signal : SIGNALS)
                sigaction(signal, &action, nullptr);
            std::set_terminate(onTerminate);
            return true;
        }
    }

    // Пути — заранее, в массивы фиксированного размера / The paths — in advance, into fixed-size arrays
    bool installCrashHandler(const CrashOptions& options)
    {
        internal::PosixCrash& state = internal::posixCrash();
        std::string folder = crashDirectory(options);

        static std::mutex installMutex;
        const std::lock_guard<std::mutex> lock(installMutex);
        if (state.installed)
            return true;
        if (options.appName.empty() || folder.size() >= sizeof(state.folder) - 64)
            return false;
        fs::createDirectories(folder);
        std::strncpy(state.folder, folder.c_str(), sizeof(state.folder) - 1);
        std::strncpy(state.prefix, options.appName.c_str(), sizeof(state.prefix) - 1);
        if (!installSignals())
            return false;
        state.installed = true;
        return true;
    }
}

#endif
