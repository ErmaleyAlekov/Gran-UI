#include "framework/test_framework.hpp"

#include <cmath>
#include <utility>

#ifdef _WIN32
# include <windows.h>
#endif

namespace test
{
    // Исключение проваленной проверки хранит готовое сообщение
    // The exception of a failed check keeps a ready message
    TestFailure::TestFailure(std::string message)
        : _message(std::move(message))
    {
    }

    // Сообщение о провале
    // The failure message
    const char* TestFailure::what() const noexcept
    {
        return _message.c_str();
    }

    // Ложное условие превращается в TestFailure с указанием файла и строки
    // A false condition becomes a TestFailure with the file and the line
    void checkTrue(bool condition, const char* expression, const char* file, int line)
    {
        if (condition)
            return;
        throw TestFailure(std::string(file) + ":" + std::to_string(line)
            + ": проверка не прошла: " + expression);
    }

    // При расхождении показываем и фактическое, и ожидаемое значение
    // On a mismatch both the actual and the expected value are shown
    void checkEqual(const std::string& actual, const std::string& expected,
        const char* expression, const char* file, int line)
    {
        if (actual == expected)
            return;
        throw TestFailure(std::string(file) + ":" + std::to_string(line)
            + ": " + expression + "\n       получено:  [" + actual
            + "]\n       ожидалось: [" + expected + "]");
    }

    // Расхождение больше допуска — провал с обоими значениями
    // A difference above the tolerance is a failure with both values
    void checkNear(const NearValues& values, const char* expression, const char* file, int line)
    {
        if (std::fabs(values.actual - values.expected) <= values.tolerance)
            return;
        throw TestFailure(std::string(file) + ":" + std::to_string(line)
            + ": " + expression + "\n       получено:  " + std::to_string(values.actual)
            + "\n       ожидалось: " + std::to_string(values.expected));
    }

    // Консоль Windows по умолчанию не в UTF-8 — переключаем для кириллицы
    // The Windows console is not in UTF-8 by default — switch it for Cyrillic
    void enableUtf8Console() noexcept
    {
#ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
#endif
    }
}
