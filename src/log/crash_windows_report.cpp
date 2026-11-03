#if defined(_WIN32)

#include "gran/log/internal/crash_windows.hpp"
#include "gran/log/log.hpp"

#include <cstdio>
#include <cwchar>

namespace gran::log::internal
{
    namespace
    {
        // Имя частых кодов исключений; прочие — только числом / The name of frequent exception codes; the others — only by number
        const char* exceptionName(DWORD code) noexcept
        {
            switch (code)
            {
                case EXCEPTION_ACCESS_VIOLATION: return "ACCESS_VIOLATION";
                case EXCEPTION_STACK_OVERFLOW: return "STACK_OVERFLOW";
                case EXCEPTION_ILLEGAL_INSTRUCTION: return "ILLEGAL_INSTRUCTION";
                case EXCEPTION_INT_DIVIDE_BY_ZERO: return "INT_DIVIDE_BY_ZERO";
                case EXCEPTION_IN_PAGE_ERROR: return "IN_PAGE_ERROR";
                case 0xC0000409: return "STACK_BUFFER_OVERRUN";
                case 0xE0000001: return "STD_TERMINATE";
                case 0xE0000002: return "ABORT";
                case 0xE0000003: return "CRT_INVALID_PARAMETER";
                default: return "EXCEPTION";
            }
        }

        // «папка\имя-ГГГГММДД-ЧЧММСС-процесс.расширение» / "folder\name-YYYYMMDD-HHMMSS-process.extension"
        HANDLE createReportFile(const CrashState& state, const SYSTEMTIME& time, const wchar_t* extension) noexcept
        {
            wchar_t path[MAX_PATH + 64];

            std::swprintf(path, MAX_PATH + 64, L"%ls\\%ls-%04u%02u%02u-%02u%02u%02u-%lu.%ls", state.folder, state.prefix,
                time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, GetCurrentProcessId(), extension);
            return CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        }

        // Минидамп с памятью, на которую ссылаются стеки, и сведениями о потоках / A minidump with the memory the stacks refer to and the thread information
        void writeDumpFile(CrashState& state, const SYSTEMTIME& time) noexcept
        {
            MINIDUMP_EXCEPTION_INFORMATION exception{};
            HANDLE file = INVALID_HANDLE_VALUE;
            MINIDUMP_TYPE type = static_cast<MINIDUMP_TYPE>(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory
                | MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules);

            if (state.writeDump == nullptr)
                return;
            file = createReportFile(state, time, L"dmp");
            if (file == INVALID_HANDLE_VALUE)
                return;
            exception.ThreadId = state.threadId;
            exception.ExceptionPointers = state.pointers;
            exception.ClientPointers = FALSE;
            state.writeDump(GetCurrentProcess(), GetCurrentProcessId(), file, type, &exception, nullptr, nullptr);
            CloseHandle(file);
        }

        // Заголовок, код, адрес, текст, стек и хвост журнала / The header, the code, the address, the text, the stack and the log tail
        void writeTextFile(CrashState& state, const SYSTEMTIME& time) noexcept
        {
            static char recent[16384 + 1];
            char line[256];
            HANDLE file = createReportFile(state, time, L"txt");
            const EXCEPTION_RECORD* record = state.pointers->ExceptionRecord;

            if (file == INVALID_HANDLE_VALUE)
                return;
            std::snprintf(line, sizeof(line), "Gran UI crash report\nprocess: %lu, thread: %lu\nexception: 0x%08lX %s\n"
                "message: %s\n", GetCurrentProcessId(), state.threadId, record->ExceptionCode,
                exceptionName(record->ExceptionCode), state.message);
            writeText(file, line);
            if (record->ExceptionAddress != nullptr)
            {
                writeText(file, "address: ");
                writeAddress(file, reinterpret_cast<DWORD64>(record->ExceptionAddress));
            }
            writeText(file, "\nstack:\n");
            writeStack(file, *state.pointers->ContextRecord);
            writeText(file, "\nlog:\n");
            recent[copyRecent(recent, sizeof(recent) - 1)] = '\0';
            writeText(file, recent);
            CloseHandle(file);
        }
    }

    // Сначала текст (он короче и важнее), затем минидамп / The text first (it is shorter and more important), then the minidump
    void writeReport(CrashState& state) noexcept
    {
        SYSTEMTIME time{};

        GetLocalTime(&time);
        if (state.pointers == nullptr)
            return;
        writeTextFile(state, time);
        writeDumpFile(state, time);
    }
}

#endif
