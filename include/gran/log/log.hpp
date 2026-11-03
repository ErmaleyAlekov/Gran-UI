#ifndef GRAN_LOG_LOG_HPP
#define GRAN_LOG_LOG_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace gran::log
{
    /*
    ** Уровень записи журнала: SEVERE — ошибка, WARNING — предупреждение, INFO — сведения, VERBOSE — подробности.
    ** Имена ERROR и DEBUG не годятся: это макросы windows.h и некоторых сборок.
    **
    ** The log record level: SEVERE — an error, WARNING — a warning, INFO — information, VERBOSE — details.
    ** The ERROR and DEBUG names do not fit: they are macros of windows.h and of some builds.
    */
    enum class Level : std::uint8_t { SEVERE, WARNING, INFO, VERBOSE };

    /*
    ** Параметры журнала. appName — имя программы: каталог и имя файла. directory — свой каталог (UTF-8);
    ** пусто — по умолчанию (defaultDirectory). Файл больше maxBytes переименовывается в .1.log (старые сдвигаются),
    ** хранится files файлов. Записи ниже level не пишутся.
    **
    ** The log parameters. appName is the program name: the directory and the file name. directory is a custom directory (UTF-8);
    ** empty — the default one (defaultDirectory). A file larger than maxBytes is renamed into .1.log (the old ones shift),
    ** files files are kept. Records below level are not written.
    */
    struct LogOptions
    {
        std::string appName;
        std::string directory;
        std::size_t maxBytes = 1024 * 1024;
        unsigned files = 5;
        Level level = Level::INFO;
    };

    // ---- log_open.cpp: журнал одного процесса (паттерн Singleton: его читает и обработчик сбоя) ----
    // ---- log_open.cpp: the log of one process (the Singleton pattern: the crash handler reads it too) ----

    /*
    ** Открыть файл журнала (каталог создаётся). Повторный вызов с тем же каталогом и именем — ничего;
    ** с другим — прежний файл закрывается. false — файл не открылся: записи идут только в кольцо и stderr.
    **
    ** Open the log file (the directory is created). A repeated call with the same directory and name — nothing;
    ** with another — the former file is closed. false — the file did not open: the records go only to the ring and stderr.
    */
    bool open(const LogOptions& options);

    // Закрыть файл (кольцо в памяти остаётся) / Close the file (the ring in memory stays)
    void close() noexcept;

    // Путь открытого файла журнала (UTF-8); пусто — журнал не открыт / The open log file path (UTF-8); empty — the log is not open
    std::string filePath();

    // Каталог журнала (UTF-8); пусто — журнал не открыт / The log directory (UTF-8); empty — the log is not open
    std::string directory();

    // ---- log_write.cpp ----

    /*
    ** Запись из любого потока: «время | уровень | поток | категория | текст». Строка попадает в кольцо
    ** последних записей (для отчёта о сбое) и в файл, если он открыт; SEVERE и WARNING дублируются в stderr.
    ** Не бросает исключений: журнал не должен ронять программу.
    **
    ** A record from any thread: "time | level | thread | category | text". The line goes into the ring of the latest
    ** records (for the crash report) and into the file if it is open; SEVERE and WARNING are duplicated to stderr.
    ** Throws no exceptions: the log must not crash the program.
    */
    void write(Level level, std::string_view category, std::string_view text) noexcept;

    // ---- log_ring.cpp ----

    /*
    ** Последние записи из кольца в buffer (не больше size байт, по порядку, без завершающего нуля); число байт.
    ** Без выделения памяти и без замка: её вызывает обработчик сбоя, когда другой поток мог держать замок.
    **
    ** The latest records from the ring into buffer (at most size bytes, in order, without a terminating zero); the byte count.
    ** Without memory allocation and without the lock: the crash handler calls it when another thread could hold the lock.
    */
    std::size_t copyRecent(char* buffer, std::size_t size) noexcept;

    // ---- log_paths.cpp ----

    // Каталог журналов по умолчанию: %LOCALAPPDATA%\<программа>\logs или $XDG_STATE_HOME/<программа>/logs
    // The default log directory: %LOCALAPPDATA%\<program>\logs or $XDG_STATE_HOME/<program>/logs
    std::string defaultDirectory(const std::string& appName);

    // Описание ОС для журнала («Windows 10.0.26300», «Linux 6.6 / Astra Linux 1.7») / The OS description for the log
    std::string systemDescription();
}

#endif
