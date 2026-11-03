#if defined(_WIN32)

#include "gran/log/internal/crash_windows.hpp"

#include <cstdio>
#include <cstring>

namespace gran::log::internal
{
    namespace
    {
        // Счётчик команд кадра (x64 — Rip, ARM64 — Pc) / The frame program counter (x64 — Rip, ARM64 — Pc)
        DWORD64& programCounter(CONTEXT& context) noexcept
        {
#if defined(_M_ARM64) || defined(__aarch64__)
            return context.Pc;
#else
            return context.Rip;
#endif
        }

        /*
        ** Листовая функция без таблицы раскрутки: адрес возврата на x64 лежит на вершине стека, на ARM64 —
        ** в регистре LR. Стек читается через ReadProcessMemory: испорченный указатель не уронит поток-помощник.
        **
        ** A leaf function without an unwind table: the return address on x64 lies at the stack top, on ARM64 —
        ** in the LR register. The stack is read through ReadProcessMemory: a damaged pointer does not crash the helper thread.
        */
        bool stepLeaf(CONTEXT& context) noexcept
        {
#if defined(_M_ARM64) || defined(__aarch64__)
            if (context.Lr == context.Pc)
                return false;
            context.Pc = context.Lr;
            return true;
#else
            DWORD64 address = 0;
            SIZE_T read = 0;

            if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(context.Rsp), &address,
                sizeof(address), &read) || read != sizeof(address))
                return false;
            context.Rip = address;
            context.Rsp += sizeof(address);
            return true;
#endif
        }
    }

    // Запись без буферизации CRT: файл закрывается сразу после отчёта / Writing without the CRT buffering: the file is closed right after the report
    void writeText(HANDLE file, const char* text) noexcept
    {
        DWORD written = 0;

        WriteFile(file, text, static_cast<DWORD>(std::strlen(text)), &written, nullptr);
    }

    // «адрес модуль+0xсмещение»; адрес вне модулей — только число / "address module+0xoffset"; an address outside the modules — only the number
    void writeAddress(HANDLE file, DWORD64 address) noexcept
    {
        HMODULE module = nullptr;
        char path[MAX_PATH] = "?";
        char line[MAX_PATH + 64];
        const char* name = path;

        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(address), &module))
            GetModuleFileNameA(module, path, MAX_PATH);
        if (std::strrchr(path, '\\') != nullptr)
            name = std::strrchr(path, '\\') + 1;
        std::snprintf(line, sizeof(line), "  0x%016llX %s+0x%llX\n", static_cast<unsigned long long>(address), name,
            static_cast<unsigned long long>(address - reinterpret_cast<DWORD64>(module)));
        writeText(file, line);
    }

    /*
    ** Раскрутка по таблицам исключений модулей (RtlLookupFunctionEntry + RtlVirtualUnwind) — без dbghelp
    ** и без символов: имена функций потом даёт отладчик или addr2line по «модуль+смещение».
    **
    ** Unwinding by the module exception tables (RtlLookupFunctionEntry + RtlVirtualUnwind) — without dbghelp
    ** and without symbols: the function names come later from a debugger or addr2line by "module+offset".
    */
    void writeStack(HANDLE file, const CONTEXT& start) noexcept
    {
        CONTEXT context = start;

        for (int frame = 0; frame < 64 && programCounter(context) != 0; ++frame)
        {
            DWORD64 pc = programCounter(context);
            DWORD64 imageBase = 0;
            void* handlerData = nullptr;
            DWORD64 establisher = 0;

            writeAddress(file, pc);
            PRUNTIME_FUNCTION function = RtlLookupFunctionEntry(pc, &imageBase, nullptr);
            if (function == nullptr)
            {
                if (!stepLeaf(context))
                    break;
                continue;
            }
            RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, pc, function, &context, &handlerData, &establisher, nullptr);
        }
    }
}

#endif
