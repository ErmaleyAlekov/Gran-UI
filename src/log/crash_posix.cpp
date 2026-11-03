#if !defined(_WIN32)

#include "gran/log/crash_report.hpp"
#include "gran/log/internal/crash_posix.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <execinfo.h>
#include <filesystem>

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

        /*
        ** Обработчик на своём стеке (sigaltstack: переполнение стека иначе не обработать). После отчёта сигнал
        ** повторяется: SA_RESETHAND уже вернул действие по умолчанию — код выхода и core dump как без нас.
        **
        ** The handler on its own stack (sigaltstack: otherwise a stack overflow cannot be handled). After the report the signal
        ** is repeated: SA_RESETHAND has already restored the default action — the exit code and the core dump as without us.
        */
        void onSignal(int signal, siginfo_t* info, void* context)
        {
            internal::writeSignalReport(signal, info, context);
            std::raise(signal);
        }

        // std::terminate: текст непойманного исключения — в отчёт, затем abort (→ SIGABRT) / std::terminate: the uncaught exception text goes into the report, then abort (→ SIGABRT)
        void onTerminate()
        {
            internal::PosixCrash& state = internal::posixCrash();
            std::exception_ptr current = std::current_exception();

            std::snprintf(state.message, sizeof(state.message), "std::terminate");
            try
            {
                if (current)
                    std::rethrow_exception(current);
            }
            catch (const std::exception& error)
            {
                std::snprintf(state.message, sizeof(state.message), "std::terminate: %s", error.what());
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
            action.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESETHAND;
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
        std::error_code error;

        if (state.installed)
            return true;
        if (options.appName.empty() || folder.size() >= sizeof(state.folder) - 64)
            return false;
        std::filesystem::create_directories(folder, error);
        std::strncpy(state.folder, folder.c_str(), sizeof(state.folder) - 1);
        std::strncpy(state.prefix, options.appName.c_str(), sizeof(state.prefix) - 1);
        if (!installSignals())
            return false;
        state.installed = true;
        return true;
    }
}

#endif
