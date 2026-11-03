#include "gran/log/internal/log_state.hpp"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <functional>
#include <iostream>
#include <thread>

namespace gran::log
{
    namespace
    {
        // Имя уровня одной ширины — столбцы журнала ровные / The level name of one width — the log columns are even
        const char* levelName(Level level) noexcept
        {
            static const char* const NAMES[] = {"SEVERE ", "WARNING", "INFO   ", "VERBOSE"};

            return NAMES[static_cast<std::size_t>(level)];
        }

        // Местное время с миллисекундами: «2026-11-03 10:20:07.123» / The local time with milliseconds: "2026-11-03 10:20:07.123"
        std::string timeStamp()
        {
            std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
            std::time_t seconds = std::chrono::system_clock::to_time_t(now);
            long long millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
            std::tm local{};
            char text[32];
            char result[40];

#if defined(_WIN32)
            localtime_s(&local, &seconds);
#else
            localtime_r(&seconds, &local);
#endif
            std::strftime(text, sizeof(text), "%Y-%m-%d %H:%M:%S", &local);
            std::snprintf(result, sizeof(result), "%s.%03lld", text, millis);
            return result;
        }

        // Короткая метка потока (4 шестнадцатеричные цифры хеша) / A short thread tag (4 hex digits of the hash)
        std::string threadTag()
        {
            char text[8];
            std::size_t hash = std::hash<std::thread::id>()(std::this_thread::get_id());

            std::snprintf(text, sizeof(text), "%04x", static_cast<unsigned>(hash & 0xFFFFu));
            return text;
        }
    }

    namespace internal
    {
        // «время | уровень | поток | категория | текст\n» / "time | level | thread | category | text\n"
        std::string formatLine(Level level, std::string_view category, std::string_view text)
        {
            std::string line = timeStamp() + " | " + levelName(level) + " | " + threadTag() + " | ";

            line.append(category.data(), category.size());
            line += " | ";
            line.append(text.data(), text.size());
            line += '\n';
            return line;
        }
    }

    /*
    ** Запись ниже уровня журнала отбрасывается. Ошибки и предупреждения дублируются в stderr — как
    ** раньше, до журнала (консольные программы и отладка их видят). Файл сбрасывается после каждой строки:
    ** при падении последние записи не теряются. Переполнение — ротация.
    **
    ** A record below the log level is dropped. Errors and warnings are duplicated to stderr — as
    ** before the log (console programs and debugging see them). The file is flushed after every line:
    ** the latest records are not lost on a crash. An overflow is a rotation.
    */
    void write(Level level, std::string_view category, std::string_view text) noexcept
    {
        try
        {
            internal::LogState& state = internal::logState();
            std::string line = internal::formatLine(level, category, text);

            if (level <= Level::WARNING)
                std::cerr << line;
            std::lock_guard<std::mutex> lock(state.mutex);
            if (level > state.options.level)
                return;
            internal::appendRing(state, line);
            if (!state.open)
                return;
            state.file << line;
            state.file.flush();
            state.size += line.size();
            if (state.size >= state.options.maxBytes)
                internal::rotate(state);
        }
        catch (...)
        {
            // Журнал не роняет программу: запись теряется / The log does not crash the program: the record is lost
        }
    }
}
