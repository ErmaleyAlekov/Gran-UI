#if defined(_WIN32)

#include "gran/log/crash_report.hpp"
#include "gran/log/internal/crash_windows.hpp"

#include <cwchar>
#include <filesystem>

namespace gran::log
{
    namespace internal
    {
        // Состояние живёт до конца процесса: фильтр может сработать при любом выходе / The state lives until the process end: the filter may fire on any exit
        CrashState& crashState() noexcept
        {
            static CrashState state;

            return state;
        }

        /*
        ** Фильтр работает в упавшем потоке, где стека может почти не остаться (переполнение стека):
        ** он только передаёт указатели помощнику и ждёт. Второй сбой (в том числе в самом отчёте)
        ** отдаётся ОС. EXCEPTION_EXECUTE_HANDLER завершает процесс без окна WER.
        **
        ** The filter works in the crashed thread where almost no stack may be left (a stack overflow):
        ** it only hands the pointers to the helper and waits. A second crash (including one in the report itself)
        ** is given to the OS. EXCEPTION_EXECUTE_HANDLER ends the process without the WER window.
        */
        LONG WINAPI crashFilter(EXCEPTION_POINTERS* pointers)
        {
            CrashState& state = crashState();

            if (InterlockedExchange(&state.busy, 1) != 0)
                return EXCEPTION_CONTINUE_SEARCH;
            state.pointers = pointers;
            state.threadId = GetCurrentThreadId();
            SetEvent(state.request);
            WaitForSingleObject(state.done, 60000);
            return EXCEPTION_EXECUTE_HANDLER;
        }
    }

    namespace
    {
        // Поток-помощник: спит до сбоя, пишет отчёт со своим целым стеком / The helper thread: sleeps until a crash, writes the report with its own whole stack
        DWORD WINAPI helperMain(void*)
        {
            internal::CrashState& state = internal::crashState();

            WaitForSingleObject(state.request, INFINITE);
            internal::writeReport(state);
            SetEvent(state.done);
            return 0;
        }

        // Пути и dbghelp — до сбоя; dbghelp не найден — будет только текстовый отчёт / The paths and dbghelp — before a crash; no dbghelp — only the text report
        bool prepareState(internal::CrashState& state, const CrashOptions& options)
        {
            std::filesystem::path folder = std::filesystem::u8path(crashDirectory(options));
            std::error_code error;
            HMODULE dbghelp = LoadLibraryW(L"dbghelp.dll");

            std::filesystem::create_directories(folder, error);
            if (folder.wstring().size() + 64 >= MAX_PATH)
                return false;
            std::wcsncpy(state.folder, folder.wstring().c_str(), MAX_PATH - 1);
            std::wcsncpy(state.prefix, std::filesystem::u8path(options.appName).wstring().c_str(), 127);
            if (dbghelp != nullptr)
                state.writeDump = reinterpret_cast<internal::MiniDumpWriteDumpFn>(
                    reinterpret_cast<void*>(GetProcAddress(dbghelp, "MiniDumpWriteDump")));
            return true;
        }
    }

    /*
    ** Установка: пути, события, поток-помощник с запасом стека, затем фильтр и перехватчики CRT.
    ** Поток-помощник ждёт весь процесс; при обычном выходе ОС его просто завершает.
    **
    ** Installing: the paths, the events, the helper thread with a stack reserve, then the filter and the CRT hooks.
    ** The helper thread waits the whole process; on a normal exit the OS simply ends it.
    */
    bool installCrashHandler(const CrashOptions& options)
    {
        internal::CrashState& state = internal::crashState();
        HANDLE helper = nullptr;

        if (state.installed)
            return true;
        if (options.appName.empty() || !prepareState(state, options))
            return false;
        state.request = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        state.done = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (state.request != nullptr && state.done != nullptr)
            helper = CreateThread(nullptr, 512 * 1024, helperMain, nullptr, 0, nullptr);
        if (helper == nullptr)
            return false;
        // Дескриптор потока не нужен: поток живёт сам до конца процесса / The thread handle is not needed: the thread lives by itself until the process end
        CloseHandle(helper);
        SetUnhandledExceptionFilter(internal::crashFilter);
        internal::installHooks();
        state.installed = true;
        return true;
    }
}

#endif
