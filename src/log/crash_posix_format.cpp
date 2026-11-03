#if !defined(_WIN32)

#include "gran/log/internal/crash_posix.hpp"

#include <cstring>
#include <unistd.h>

namespace gran::log::internal
{
    // write может записать меньше — дописываем остаток; ошибку некому сообщить / write may write less — the rest is written; there is nobody to report an error to
    void putText(int fd, const char* text) noexcept
    {
        std::size_t left = std::strlen(text);

        while (left > 0)
        {
            ssize_t written = ::write(fd, text, left);
            if (written <= 0)
                return;
            text += written;
            left -= static_cast<std::size_t>(written);
        }
    }

    // Цифры собираются с конца во временном массиве на стеке / The digits are collected from the end in a temporary stack array
    void putNumber(int fd, std::uint64_t value, unsigned base) noexcept
    {
        static const char DIGITS[] = "0123456789abcdef";
        char text[24];
        std::size_t position = sizeof(text) - 1;

        text[position] = '\0';
        do
        {
            text[--position] = DIGITS[value % base];
            value /= base;
        }
        while (value != 0 && position > 2);
        if (base == 16)
        {
            text[--position] = 'x';
            text[--position] = '0';
        }
        putText(fd, text + position);
    }

    // Обрезка по ёмкости: длинный путь не переполнит буфер / Truncation by the capacity: a long path does not overflow the buffer
    std::size_t appendText(char* buffer, std::size_t capacity, std::size_t position, const char* text) noexcept
    {
        while (*text != '\0' && position + 1 < capacity)
            buffer[position++] = *text++;
        buffer[position] = '\0';
        return position;
    }

    // Десятичное число — через ту же обрезку / A decimal number — through the same truncation
    std::size_t appendNumber(char* buffer, std::size_t capacity, std::size_t position, std::uint64_t value) noexcept
    {
        char text[24];
        std::size_t start = sizeof(text) - 1;

        text[start] = '\0';
        do
        {
            text[--start] = static_cast<char>('0' + value % 10);
            value /= 10;
        }
        while (value != 0 && start > 0);
        return appendText(buffer, capacity, position, text + start);
    }

    // Сигналы, которые перехватывает обработчик / The signals the handler catches
    const char* signalName(int signal) noexcept
    {
        switch (signal)
        {
            case SIGSEGV: return "SIGSEGV";
            case SIGBUS: return "SIGBUS";
            case SIGILL: return "SIGILL";
            case SIGFPE: return "SIGFPE";
            case SIGABRT: return "SIGABRT";
            default: return "SIGNAL";
        }
    }
}

#endif
