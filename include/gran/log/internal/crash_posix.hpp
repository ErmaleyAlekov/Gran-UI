#ifndef GRAN_LOG_INTERNAL_CRASH_POSIX_HPP
#define GRAN_LOG_INTERNAL_CRASH_POSIX_HPP

// Только для файлов Linux (crash_posix*.cpp) / Only for the Linux files (crash_posix*.cpp)
#if !defined(_WIN32)

#include <csignal>
#include <cstddef>
#include <cstdint>

namespace gran::log::internal
{
    /*
    ** Состояние обработчика сигналов: только массивы фиксированного размера — в обработчике сигнала
    ** нельзя выделять память и брать замки (допустимы лишь async-signal-safe вызовы).
    **
    ** The signal handler state: only fixed-size arrays — in a signal handler one cannot allocate memory
    ** and take locks (only async-signal-safe calls are allowed).
    */
    struct PosixCrash
    {
        char folder[1024] = {};
        char prefix[128] = {};
        char message[512] = {};
        bool installed = false;
    };

    // ---- crash_posix.cpp ----

    // Единственное состояние обработчика / The only handler state
    PosixCrash& posixCrash() noexcept;

    // ---- crash_posix_report.cpp ----

    // Отчёт о сигнале: .txt (сигнал, регистры, backtrace, maps, журнал) и .stack (байты стека) / The signal report: .txt (the signal, the registers, backtrace, maps, the log) and .stack (the stack bytes)
    void writeSignalReport(int signal, siginfo_t* info, void* context) noexcept;

    // ---- crash_posix_format.cpp: вывод без выделений памяти / output without memory allocation ----

    // Строка в дескриптор / A string into a descriptor
    void putText(int fd, const char* text) noexcept;

    // Число в дескриптор: base 10 или 16 (с «0x») / A number into a descriptor: base 10 or 16 (with "0x")
    void putNumber(int fd, std::uint64_t value, unsigned base) noexcept;

    // Дописать строку в буфер; новая позиция (буфер всегда с завершающим нулём) / Append a string to a buffer; the new position (the buffer always has a terminating zero)
    std::size_t appendText(char* buffer, std::size_t capacity, std::size_t position, const char* text) noexcept;

    // Дописать десятичное число в буфер; новая позиция / Append a decimal number to a buffer; the new position
    std::size_t appendNumber(char* buffer, std::size_t capacity, std::size_t position, std::uint64_t value) noexcept;

    // Имя сигнала (SIGSEGV…) / The signal name (SIGSEGV…)
    const char* signalName(int signal) noexcept;
}

#endif

#endif
