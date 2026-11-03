#ifndef GRAN_LOG_INTERNAL_LOG_STATE_HPP
#define GRAN_LOG_INTERNAL_LOG_STATE_HPP

#include "gran/log/log.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>

namespace gran::log::internal
{
    // Размер кольца последних записей (байт) / The size of the ring of the latest records (bytes)
    constexpr std::size_t RING_BYTES = 16384;

    /*
    ** Состояние журнала процесса. Кольцо — массив без выделений памяти: его копирует обработчик сбоя
    ** без замка (поток, державший замок, мог упасть). Остальное — под mutex.
    **
    ** The process log state. The ring is an array without memory allocation: the crash handler copies it
    ** without the lock (the thread that held the lock could crash). The rest is under mutex.
    */
    struct LogState
    {
        std::mutex mutex;
        LogOptions options;
        std::filesystem::path folder;
        std::filesystem::path path;
        std::ofstream file;
        std::uintmax_t size = 0;
        bool open = false;
        char ring[RING_BYTES] = {};
        std::size_t ringHead = 0;
        bool ringFull = false;
    };

    // ---- log_open.cpp ----

    // Единственное состояние журнала процесса / The only log state of the process
    LogState& logState() noexcept;

    // ---- log_rotate.cpp ----

    // Сдвинуть файлы name.log → name.1.log → … и открыть пустой name.log (под замком) / Shift the files name.log → name.1.log → … and open an empty name.log (under the lock)
    void rotate(LogState& state);

    // Открыть файл на дозапись, узнать его размер (под замком); false — не открылся / Open the file for appending, learn its size (under the lock); false — it did not open
    bool openFile(LogState& state);

    // ---- log_ring.cpp ----

    // Дописать строку в кольцо (под замком) / Append a line to the ring (under the lock)
    void appendRing(LogState& state, std::string_view line) noexcept;

    // ---- log_write.cpp ----

    // Строка записи с временем, уровнем, потоком и категорией (с переводом строки) / The record line with the time, the level, the thread and the category (with a line feed)
    std::string formatLine(Level level, std::string_view category, std::string_view text);
}

#endif
