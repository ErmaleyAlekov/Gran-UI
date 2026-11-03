#ifndef GRAN_LOG_INTERNAL_CRASH_WINDOWS_HPP
#define GRAN_LOG_INTERNAL_CRASH_WINDOWS_HPP

// Только для файлов Windows (crash_windows*.cpp) / Only for the Windows files (crash_windows*.cpp)
#if defined(_WIN32)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dbghelp.h>

namespace gran::log::internal
{
    // MiniDumpWriteDump из dbghelp.dll (загружается при установке, без связи при сборке) / MiniDumpWriteDump from dbghelp.dll (loaded on installing, without a link-time dependency)
    typedef BOOL (WINAPI* MiniDumpWriteDumpFn)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION,
        PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION);

    /*
    ** Состояние обработчика сбоя. Всё готовится при установке: пути, dbghelp, события и поток-помощник —
    ** в момент сбоя ничего не создаётся (куча и стек упавшего потока могут быть испорчены).
    ** busy — второй сбой во время отчёта уходит ОС.
    **
    ** The crash handler state. Everything is prepared on installing: the paths, dbghelp, the events and the helper thread —
    ** nothing is created at the crash moment (the heap and the stack of the crashed thread may be damaged).
    ** busy — a second crash during the report goes to the OS.
    */
    struct CrashState
    {
        wchar_t folder[MAX_PATH] = {};
        wchar_t prefix[128] = {};
        MiniDumpWriteDumpFn writeDump = nullptr;
        HANDLE request = nullptr;
        HANDLE done = nullptr;
        EXCEPTION_POINTERS* pointers = nullptr;
        DWORD threadId = 0;
        char message[512] = {};
        volatile LONG busy = 0;
        bool installed = false;
    };

    // ---- crash_windows.cpp ----

    // Единственное состояние обработчика / The only handler state
    CrashState& crashState() noexcept;

    // Фильтр необработанных исключений: отчёт пишет поток-помощник, затем процесс завершается / The unhandled exception filter: the helper thread writes the report, then the process ends
    LONG WINAPI crashFilter(EXCEPTION_POINTERS* pointers);

    // ---- crash_windows_hooks.cpp ----

    // Сбой без исключения SEH (terminate, abort, CRT): контекст текущего потока и свой код / A crash without an SEH exception (terminate, abort, CRT): the current thread context and an own code
    [[noreturn]] void syntheticCrash(DWORD code, const char* message) noexcept;

    // Подключить std::terminate, чисто виртуальный вызов, неверный параметр CRT и SIGABRT / Hook std::terminate, a pure virtual call, a CRT invalid parameter and SIGABRT
    void installHooks() noexcept;

    // ---- crash_windows_report.cpp ----

    // Минидамп и текстовый отчёт (из потока-помощника) / The minidump and the text report (from the helper thread)
    void writeReport(CrashState& state) noexcept;

    // ---- crash_windows_stack.cpp ----

    // Строка в файл отчёта / A string into the report file
    void writeText(HANDLE file, const char* text) noexcept;

    // Адрес: модуль+смещение (адреса в минидампе и у отладчика совпадут) / An address: module+offset (the addresses in the minidump and at the debugger match)
    void writeAddress(HANDLE file, DWORD64 address) noexcept;

    // Стек упавшего потока по его контексту: RtlVirtualUnwind, до 64 кадров / The crashed thread stack by its context: RtlVirtualUnwind, up to 64 frames
    void writeStack(HANDLE file, const CONTEXT& start) noexcept;
}

#endif

#endif
