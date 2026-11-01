#ifndef GRAN_DATA_JSON_READER_HPP
#define GRAN_DATA_JSON_READER_HPP

#include <cstddef>
#include <cstdint>
#include <istream>
#include <string>
#include <vector>

namespace gran::data
{
    // Лексема JSON для читателя-вытягивания / A JSON token for the pull reader
    enum class JsonToken : std::uint8_t
    {
        BEGIN_OBJECT, END_OBJECT, BEGIN_ARRAY, END_ARRAY, NAME, STRING, NUMBER, BOOLEAN, NUL, END
    };

    /*
    ** Свой читатель JSON (RFC 8259) в режиме вытягивания, как StAX у XML: next() отдаёт следующую
    ** лексему, текст значения — value(). Файл читается кусками по 64 КБ, дерево не строится, поэтому
    ** ленивая подгрузка может остановиться после N записей и продолжить позже с того же места.
    ** Строки — UTF-8 с \uXXXX и суррогатными парами, числа отдаются текстом (как поля записей).
    ** Ошибка — DataException с файлом, строкой и столбцом (столбец — в символах).
    **
    ** An own JSON reader (RFC 8259) in the pull mode, like StAX for XML: next() returns the next
    ** token, the value text is value(). The file is read in 64 KB chunks and no tree is built, so
    ** lazy loading can stop after N records and continue later from the same place.
    ** Strings are UTF-8 with \uXXXX and surrogate pairs, numbers are returned as text (as the record fields).
    ** An error is DataException with the file, the line and the column (the column is in characters).
    */
    class JsonReader
    {
    public:
        // ---- json_reader.cpp ----

        // input должен жить дольше читателя; source — имя файла для сообщений / input must outlive the reader; source is the file name for messages
        JsonReader(std::istream& input, std::string source);

        // Следующая лексема; END — документ закончился / The next token; END — the document ended
        JsonToken next();

        // Текст NAME, STRING, NUMBER и BOOLEAN ("true"/"false") / The text of NAME, STRING, NUMBER and BOOLEAN ("true"/"false")
        const std::string& value() const noexcept;

        // Пропустить значение, начатое лексемой token (у скаляра — ничего) / Skip the value started by the token (nothing for a scalar)
        void skip(JsonToken token);

        // Ошибка в текущей позиции с текстом detail / An error at the current position with the detail text
        [[noreturn]] void fail(const std::string& detail) const;

    private:
        // Состояние открытого объекта или массива / The state of an open object or array
        enum class Expect : std::uint8_t { FIRST, NEXT, VALUE, SEPARATOR };
        struct Frame
        {
            bool object;
            Expect expect;
        };

        // ---- json_reader_values.cpp ----

        // Значение в позиции, где оно ожидается / A value at a position where it is expected
        JsonToken readValue();

        // Следующая лексема внутри объекта и внутри массива / The next token inside an object and inside an array
        JsonToken nextInObject(Frame& frame);
        JsonToken nextInArray(Frame& frame);

        // Закрыть контейнер (}, ]) / Close a container (}, ])
        JsonToken close(JsonToken token);

        // Открыть контейнер ({, [) с проверкой глубины / Open a container ({, [) with a depth check
        JsonToken open(bool object);

        // ---- json_reader_scan.cpp ----

        // Текущий символ без продвижения; -1 — конец файла / The current character without advancing; -1 — the end of file
        int peek();

        // Взять символ (с учётом строки и столбца); конец файла — ошибка / Take a character (keeping the line and the column); the end of file is an error
        char take();

        // Пропустить пробельные символы JSON / Skip the JSON whitespace
        void skipSpace();

        // Ожидаемый символ, иначе ошибка / The expected character, otherwise an error
        void expect(char symbol);

        // Неожиданный символ (или конец файла) — ошибка / An unexpected character (or the end of file) — an error
        [[noreturn]] void unexpected();

        // ---- json_reader_text.cpp ----

        // Строка после открывающей кавычки / A string after the opening quote
        void readString();

        // Escape-последовательность после обратной косой черты / An escape sequence after a backslash
        void readEscape();

        // Четыре шестнадцатеричные цифры \uXXXX / Four hexadecimal digits \uXXXX
        char32_t readHex();

        // Число по грамматике RFC 8259 / A number by the RFC 8259 grammar
        void readNumber();

        // ---- json_reader_word.cpp ----

        // Слово true, false или null / The word true, false or null
        JsonToken readWord();

        // Одна или больше цифр в _value; ни одной — ошибка / One or more digits into _value; none — an error
        void readDigits();

        std::istream& _input;
        std::string _source;
        std::vector<char> _buffer;
        std::size_t _position = 0;
        std::size_t _size = 0;
        std::size_t _line = 1;
        std::size_t _column = 1;
        std::vector<Frame> _frames;
        std::string _value;
        bool _started = false;
    };
}

#endif
