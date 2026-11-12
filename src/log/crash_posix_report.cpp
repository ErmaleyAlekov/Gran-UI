#if !defined(_WIN32)

#include "gran/log/internal/crash_posix.hpp"
#include "gran/log/log.hpp"

#include <ctime>
#include <execinfo.h>
#include <fcntl.h>
#include <ucontext.h>
#include <unistd.h>

namespace gran::log::internal
{
    namespace
    {
        // «папка/имя-секунды-процесс.расширение»; -1 — файл не открылся / "folder/name-seconds-process.extension"; -1 — the file did not open
        int openReport(const PosixCrash& state, std::time_t seconds, const char* extension) noexcept
        {
            char path[sizeof(state.folder) + sizeof(state.prefix) + 64];
            std::size_t position = appendText(path, sizeof(path), 0, state.folder);

            position = appendText(path, sizeof(path), position, "/");
            position = appendText(path, sizeof(path), position, state.prefix);
            position = appendText(path, sizeof(path), position, "-");
            position = appendNumber(path, sizeof(path), position, static_cast<std::uint64_t>(seconds));
            position = appendText(path, sizeof(path), position, "-");
            position = appendNumber(path, sizeof(path), position, static_cast<std::uint64_t>(getpid()));
            position = appendText(path, sizeof(path), position, ".");
            appendText(path, sizeof(path), position, extension);
            return ::open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
        }

        /*
        ** Счётчик команд, указатель стека и кадра из контекста сигнала (x86-64 и AArch64; на других
        ** архитектурах — без регистров). Возвращает указатель стека для дампа байтов.
        **
        ** The program counter, the stack and frame pointers from the signal context (x86-64 and AArch64; on other
        ** architectures — without registers). Returns the stack pointer for the byte dump.
        */
        std::uint64_t writeRegisters(int fd, void* context) noexcept
        {
            const ucontext_t* registers = static_cast<const ucontext_t*>(context);
            std::uint64_t values[3] = {0, 0, 0};

#if defined(__x86_64__)
            values[0] = static_cast<std::uint64_t>(registers->uc_mcontext.gregs[REG_RIP]);
            values[1] = static_cast<std::uint64_t>(registers->uc_mcontext.gregs[REG_RSP]);
            values[2] = static_cast<std::uint64_t>(registers->uc_mcontext.gregs[REG_RBP]);
#elif defined(__aarch64__)
            values[0] = registers->uc_mcontext.pc;
            values[1] = registers->uc_mcontext.sp;
            values[2] = registers->uc_mcontext.regs[29];
#else
            (void)registers;
#endif
            putText(fd, "pc: ");
            putNumber(fd, values[0], 16);
            putText(fd, "\nsp: ");
            putNumber(fd, values[1], 16);
            putText(fd, "\nfp: ");
            putNumber(fd, values[2], 16);
            putText(fd, "\n");
            return values[1];
        }

        // Копия /proc/self/maps: по ней адреса backtrace переводятся в «модуль+смещение» для addr2line / A copy of /proc/self/maps: by it the backtrace addresses become "module+offset" for addr2line
        void copyMaps(int fd) noexcept
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

        /*
        ** До 64 КБ стека от sp — в файл .stack. Чтение за концом стека не роняет обработчик: write
        ** с недоступным адресом возвращает EFAULT (память проверяет ядро), и запись просто кончается.
        ** Куски не пересекают границу страницы 4 КБ (границы отображений ей кратны): первый — от sp до конца
        ** его страницы, дальше — целые страницы. Иначе при sp ближе 4 КБ к верху стека первый же кусок
        ** задевал недоступную страницу, write давал EFAULT, и файл оставался пустым (зависело от ASLR).
        **
        ** Up to 64 KB of the stack from sp go into the .stack file. Reading past the stack end does not crash the handler: write
        ** with an unreachable address returns EFAULT (the kernel checks the memory), and the writing simply ends.
        ** The pieces do not cross a 4 KB page boundary (mapping boundaries are multiples of it): the first one goes from sp to the end
        ** of its page, then whole pages. Otherwise with sp closer than 4 KB to the stack top the very first piece
        ** touched an unreachable page, write gave EFAULT, and the file stayed empty (it depended on ASLR).
        */
        void writeStackBytes(const PosixCrash& state, std::time_t seconds, std::uint64_t pointer) noexcept
        {
            const std::uint64_t page = 4096;
            const std::uint64_t end = pointer + 16 * page;
            if (pointer == 0)
                return;
            int fd = openReport(state, seconds, "stack");
            if (fd < 0)
                return;
            while (pointer < end)
            {
                // Длина до конца текущей страницы / The length up to the end of the current page
                std::size_t length = static_cast<std::size_t>(page - pointer % page);
                const char* bytes = reinterpret_cast<const char*>(static_cast<std::uintptr_t>(pointer));

                if (::write(fd, bytes, length) != static_cast<ssize_t>(length))
                    break;
                pointer += length;
            }
            ::close(fd);
        }
    }

    // Только async-signal-safe вызовы; backtrace подгружен заранее при установке / Only async-signal-safe calls; backtrace is loaded in advance on installing
    void writeSignalReport(int signal, siginfo_t* info, void* context) noexcept
    {
        static char recent[16384 + 1];
        static void* frames[64];
        PosixCrash& state = posixCrash();
        std::time_t seconds = std::time(nullptr);
        int fd = openReport(state, seconds, "txt");

        if (fd < 0)
            return;
        putText(fd, "Gran UI crash report\nsignal: ");
        putNumber(fd, static_cast<std::uint64_t>(signal), 10);
        putText(fd, " ");
        putText(fd, signalName(signal));
        putText(fd, "\naddress: ");
        putNumber(fd, reinterpret_cast<std::uintptr_t>(info->si_addr), 16);
        putText(fd, "\nmessage: ");
        putText(fd, state.message);
        putText(fd, "\n");
        std::uint64_t pointer = writeRegisters(fd, context);
        putText(fd, "\nbacktrace:\n");
        backtrace_symbols_fd(frames, backtrace(frames, 64), fd);
        putText(fd, "\nmaps:\n");
        copyMaps(fd);
        putText(fd, "\nlog:\n");
        recent[copyRecent(recent, sizeof(recent) - 1)] = '\0';
        putText(fd, recent);
        ::close(fd);
        writeStackBytes(state, seconds, pointer);
    }
}

#endif
