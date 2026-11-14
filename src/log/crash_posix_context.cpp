#if !defined(_WIN32)

#include "gran/log/internal/crash_posix.hpp"

#include <unistd.h>

/*
** Всё, что в отчёте о сигнале зависит от ОС и процессора, собрано здесь: остальные файлы crash_posix*.cpp
** одинаковы на Linux и macOS. На macOS <ucontext.h> без _XOPEN_SOURCE — ошибка компиляции (устаревшие
** функции makecontext/swapcontext), а сам тип ucontext_t даёт <sys/ucontext.h>; регистры там лежат
** в uc_mcontext->__ss, а не в uc_mcontext.gregs, и файла /proc/self/maps нет — модули берутся у dyld.
**
** Everything in the signal report that depends on the OS and the processor is gathered here: the other
** crash_posix*.cpp files are the same on Linux and macOS. On macOS <ucontext.h> without _XOPEN_SOURCE is a compile
** error (the deprecated makecontext/swapcontext functions), and the ucontext_t type itself comes from
** <sys/ucontext.h>; the registers live in uc_mcontext->__ss, not in uc_mcontext.gregs, and there is no
** /proc/self/maps file — the modules are taken from dyld.
*/
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <sys/ucontext.h>
#else
#include <fcntl.h>
#include <ucontext.h>
#endif

namespace gran::log::internal
{
    /*
    ** На AArch64 macOS поля состояния потока могут быть «непрозрачными» (подписанные указатели, arm64e):
    ** читаем только через макросы __darwin_arm_thread_state64_get_*, они снимают подпись сами.
    **
    ** On AArch64 macOS the thread state fields may be "opaque" (signed pointers, arm64e): they are read
    ** only through the __darwin_arm_thread_state64_get_* macros, which strip the signature themselves.
    */
    SignalRegisters readRegisters(const void* context) noexcept
    {
        const ucontext_t* state = static_cast<const ucontext_t*>(context);
        SignalRegisters registers;

#if defined(__APPLE__) && defined(__x86_64__)
        registers.pc = static_cast<std::uint64_t>(state->uc_mcontext->__ss.__rip);
        registers.sp = static_cast<std::uint64_t>(state->uc_mcontext->__ss.__rsp);
        registers.fp = static_cast<std::uint64_t>(state->uc_mcontext->__ss.__rbp);
#elif defined(__APPLE__) && (defined(__aarch64__) || defined(__arm64__))
        registers.pc = static_cast<std::uint64_t>(__darwin_arm_thread_state64_get_pc(state->uc_mcontext->__ss));
        registers.sp = static_cast<std::uint64_t>(__darwin_arm_thread_state64_get_sp(state->uc_mcontext->__ss));
        registers.fp = static_cast<std::uint64_t>(__darwin_arm_thread_state64_get_fp(state->uc_mcontext->__ss));
#elif defined(__x86_64__)
        registers.pc = static_cast<std::uint64_t>(state->uc_mcontext.gregs[REG_RIP]);
        registers.sp = static_cast<std::uint64_t>(state->uc_mcontext.gregs[REG_RSP]);
        registers.fp = static_cast<std::uint64_t>(state->uc_mcontext.gregs[REG_RBP]);
#elif defined(__aarch64__)
        registers.pc = state->uc_mcontext.pc;
        registers.sp = state->uc_mcontext.sp;
        registers.fp = state->uc_mcontext.regs[29];
#else
        (void)state;
#endif
        return registers;
    }

#if defined(__APPLE__)
    /*
    ** Образы dyld: адрес заголовка Mach-O — адрес загрузки модуля (для atos -l). Функции _dyld_* формально не
    ** входят в async-signal-safe, но только читают готовый список образов, без выделений памяти и замков
    ** на этом пути; так же поступают известные обработчики сбоев на macOS.
    **
    ** The dyld images: the Mach-O header address is the module load address (for atos -l). The _dyld_* functions are
    ** formally not async-signal-safe, but they only read the ready image list, without memory allocation and locks
    ** on this path; the well-known crash handlers on macOS do the same.
    */
    void writeModules(int fd) noexcept
    {
        const std::uint32_t count = _dyld_image_count();

        for (std::uint32_t index = 0; index < count; ++index)
        {
            const char* name = _dyld_get_image_name(index);

            // Образ мог выгрузиться между вызовами: тогда имени нет / The image may unload between the calls: then there is no name
            if (name == nullptr)
                continue;
            putNumber(fd, reinterpret_cast<std::uintptr_t>(_dyld_get_image_header(index)), 16);
            putText(fd, " ");
            putText(fd, name);
            putText(fd, "\n");
        }
    }
#else
    // Копия /proc/self/maps: по ней адреса backtrace переводятся в «модуль+смещение» для addr2line / A copy of /proc/self/maps: by it the backtrace addresses become "module+offset" for addr2line
    void writeModules(int fd) noexcept
    {
        char buffer[4096];
        int maps = ::open("/proc/self/maps", O_RDONLY | O_CLOEXEC);
        ssize_t count = 0;

        if (maps < 0)
            return;
        while ((count = ::read(maps, buffer, sizeof(buffer))) > 0)
        {
            if (::write(fd, buffer, static_cast<std::size_t>(count)) != count)
                break;
        }
        ::close(maps);
    }
#endif
}

#endif
