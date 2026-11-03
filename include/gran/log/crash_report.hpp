#ifndef GRAN_LOG_CRASH_REPORT_HPP
#define GRAN_LOG_CRASH_REPORT_HPP

#include <string>

namespace gran::log
{
    /*
    ** Параметры отчёта о сбое: appName — начало имён файлов, directory — каталог (UTF-8; пусто — «crashes»
    ** рядом с каталогом журнала по умолчанию).
    **
    ** The crash report parameters: appName is the file name prefix, directory is the directory (UTF-8; empty — "crashes"
    ** beside the default log directory).
    */
    struct CrashOptions
    {
        std::string appName;
        std::string directory;
    };

    /*
    ** Перехват падения процесса. Windows: необработанное исключение SEH, std::terminate, abort, неверный параметр CRT,
    ** чисто виртуальный вызов → минидамп (MiniDumpWriteDump, dbghelp.dll загружается заранее) и текстовый отчёт
    ** со стеком из потока-помощника (у упавшего потока может не быть стека). Linux: SIGSEGV, SIGBUS, SIGILL, SIGFPE,
    ** SIGABRT на своём стеке сигналов → текстовый отчёт (сигнал, адрес, регистры, backtrace, /proc/self/maps)
    ** и сырые байты стека в .stack; затем сигнал повторяется (код выхода и core dump как без обработчика).
    ** В отчёт идут последние записи журнала. Повторный вызов — ничего; false — установить не удалось.
    **
    ** Catching a process crash. Windows: an unhandled SEH exception, std::terminate, abort, a CRT invalid parameter,
    ** a pure virtual call → a minidump (MiniDumpWriteDump, dbghelp.dll is loaded in advance) and a text report
    ** with the stack from a helper thread (the crashed thread may have no stack left). Linux: SIGSEGV, SIGBUS, SIGILL, SIGFPE,
    ** SIGABRT on an own signal stack → a text report (the signal, the address, the registers, backtrace, /proc/self/maps)
    ** and the raw stack bytes in .stack; then the signal is repeated (the exit code and the core dump as without the handler).
    ** The latest log records go into the report. A repeated call — nothing; false — it could not be installed.
    */
    bool installCrashHandler(const CrashOptions& options);

    // ---- crash_previous.cpp ----

    // Каталог отчётов о сбое (UTF-8) / The crash report directory (UTF-8)
    std::string crashDirectory(const CrashOptions& options);

    /*
    ** Самый новый отчёт (.txt) каталога, ещё не показанный программе; он запоминается как показанный
    ** (файл .seen), повторный вызов даёт "". Так программа один раз предлагает отправить отчёт после сбоя.
    **
    ** The newest report (.txt) of the directory not shown to the program yet; it is remembered as shown
    ** (the .seen file), a repeated call gives "". This way the program offers to send the report once after a crash.
    */
    std::string previousCrash(const std::string& directory);
}

#endif
