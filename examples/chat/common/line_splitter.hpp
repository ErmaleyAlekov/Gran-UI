#ifndef CHAT_COMMON_LINE_SPLITTER_HPP
#define CHAT_COMMON_LINE_SPLITTER_HPP

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace chat
{
    /*
    ** Разрезает поток байтов на строки NDJSON (одна строка — один объект JSON). Кусок может
    ** оборваться посреди строки и даже посреди символа UTF-8 — незаконченная строка ждёт
    ** следующего куска. CR перед LF отбрасывается, пустые строки пропускаются.
    **
    ** Cuts a byte stream into NDJSON lines (one line — one JSON object). A piece may
    ** break in the middle of a line and even in the middle of a UTF-8 character — an unfinished line waits
    ** for the next piece. A CR before LF is dropped, empty lines are skipped.
    */
    class LineSplitter
    {
    public:
        // Предел длины строки защищает память от сервера без переводов строк / The line length limit protects the memory from a server without line breaks
        explicit LineSplitter(std::size_t maxLine = 1024 * 1024);

        // Готовые строки куска дописываются в lines; длиннее предела — ProtocolException / The ready lines of the piece are appended to lines; longer than the limit — ProtocolException
        void feed(std::string_view data, std::vector<std::string>& lines);

        // Конец потока: последняя строка без LF (пустая — если её нет) / The stream end: the last line without LF (empty — if there is none)
        std::string flush();

    private:
        std::string _pending;
        std::size_t _maxLine;
    };
}

#endif
