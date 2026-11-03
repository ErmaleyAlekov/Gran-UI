#if defined(_WIN32)

#include "gran/log/internal/crash_windows.hpp"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>

namespace gran::log::internal
{
    namespace
    {
        // Свои коды сбоев без исключения SEH (бит 29 — «код приложения») / Own crash codes without an SEH exception (bit 29 — "an application code")
        constexpr DWORD CODE_TERMINATE = 0xE0000001;
        constexpr DWORD CODE_ABORT = 0xE0000002;
        constexpr DWORD CODE_INVALID_PARAMETER = 0xE0000003;

        /*
        ** std::terminate (и чисто виртуальный вызов в MSVC): текст исключения, которое никто не поймал,
        ** если оно есть. Буфер статический — куча может быть испорчена.
        **
        ** std::terminate (and a pure virtual call in MSVC): the text of the exception nobody caught,
        ** if there is one. The buffer is static — the heap may be damaged.
        */
        void onTerminate()
        {
            static char text[512];
            std::exception_ptr current = std::current_exception();

            std::snprintf(text, sizeof(text), "std::terminate");
            try
            {
                if (current)
                    std::rethrow_exception(current);
            }
            catch (const std::exception& error)
            {
                std::snprintf(text, sizeof(text), "std::terminate: %s", error.what());
            }
            catch (...)
            {
            }
            syntheticCrash(CODE_TERMINATE, text);
        }

        // Неверный параметр функции CRT (по умолчанию CRT молча завершает процесс) / A CRT function invalid parameter (by default the CRT silently ends the process)
        void onInvalidParameter(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t)
        {
            syntheticCrash(CODE_INVALID_PARAMETER, "CRT invalid parameter");
        }

        // abort(): обработчик SIGABRT не возвращается — отчёт и выход / abort(): the SIGABRT handler does not return — the report and the exit
        void onAbort(int)
        {
            syntheticCrash(CODE_ABORT, "abort");
        }
    }

    /*
    ** Контекст текущего потока (RtlCaptureContext) вместо контекста исключения; дальше — тот же путь,
    ** что у SEH. Процесс завершается с кодом сбоя.
    **
    ** The current thread context (RtlCaptureContext) instead of an exception context; then — the same path
    ** as for SEH. The process ends with the crash code.
    */
    void syntheticCrash(DWORD code, const char* message) noexcept
    {
        CrashState& state = crashState();
        CONTEXT context{};
        EXCEPTION_RECORD record{};
        EXCEPTION_POINTERS pointers{&record, &context};

        if (message != nullptr)
            std::strncpy(state.message, message, sizeof(state.message) - 1);
        RtlCaptureContext(&context);
        record.ExceptionCode = code;
        if (state.installed)
            crashFilter(&pointers);
        TerminateProcess(GetCurrentProcess(), code);
        for (;;)
            Sleep(INFINITE);
    }

    // Перехватчики, которые иначе обходят фильтр SEH / The hooks that otherwise bypass the SEH filter
    void installHooks() noexcept
    {
        std::set_terminate(onTerminate);
        _set_invalid_parameter_handler(onInvalidParameter);
#if defined(_MSC_VER)
        _set_purecall_handler(onTerminate);
#endif
        std::signal(SIGABRT, onAbort);
    }
}

#endif
