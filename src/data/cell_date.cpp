#include "gran/data/internal/cell_formats.hpp"

namespace gran::data::internal
{
    namespace
    {
        // Части даты / The date parts
        struct DateParts
        {
            int year = 0;
            int month = 0;
            int day = 0;
        };

        /*
        ** Число из минимум min и максимум max цифр с позиции at; false — цифр меньше min. Лишние цифры
        ** не читаются: их отвергает следующая проверка (разделитель или конец текста).
        **
        ** A number of at least min and at most max digits from at; false — fewer than min digits. Extra digits
        ** are not read: the next check rejects them (a separator or the text end).
        */
        bool readPart(std::string_view text, std::size_t& at, std::size_t min, std::size_t max, int& value)
        {
            std::size_t start = at;

            value = 0;
            while (at < text.size() && text[at] >= '0' && text[at] <= '9' && at - start < max)
                value = value * 10 + (text[at++] - '0');
            return at - start >= min;
        }

        // Разделитель separator на месте at (и шаг за него) / The separator at at (and a step past it)
        bool skip(std::string_view text, std::size_t& at, char separator) noexcept
        {
            if (at >= text.size() || text[at] != separator)
                return false;
            ++at;
            return true;
        }

        // Такой день есть в этом месяце (високосный год — по григорианскому календарю) / Such a day exists in this month (a leap year — by the Gregorian calendar)
        bool exists(const DateParts& date) noexcept
        {
            static const int DAYS[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

            if (date.year < 1 || date.month < 1 || date.month > 12 || date.day < 1)
                return false;
            bool leap = (date.year % 4 == 0 && date.year % 100 != 0) || date.year % 400 == 0;
            int last = DAYS[date.month - 1];
            if (date.month == 2 && leap)
                last = 29;
            return date.day <= last;
        }

        // Число с ведущими нулями до width цифр / A number with leading zeros up to width digits
        void appendPadded(std::string& result, int value, std::size_t width)
        {
            std::string digits = std::to_string(value);

            if (digits.size() < width)
                result.append(width - digits.size(), '0');
            result += digits;
        }
    }

    // Вид даты выбирается по первому разделителю: «-» — ГГГГ-ММ-ДД, «.» — ДД.ММ.ГГГГ / The date form is chosen by the first separator: "-" — YYYY-MM-DD, "." — DD.MM.YYYY
    bool normalizeDate(std::string_view text, std::string& result)
    {
        DateParts date;
        std::size_t at = 0;
        bool iso = text.find('-') != std::string_view::npos;
        bool read = false;

        if (iso)
            read = readPart(text, at, 4, 4, date.year) && skip(text, at, '-') && readPart(text, at, 1, 2, date.month)
                && skip(text, at, '-') && readPart(text, at, 1, 2, date.day);
        else
            read = readPart(text, at, 1, 2, date.day) && skip(text, at, '.') && readPart(text, at, 1, 2, date.month)
                && skip(text, at, '.') && readPart(text, at, 4, 4, date.year);
        if (!read || at != text.size() || !exists(date))
            return false;
        result.clear();
        appendPadded(result, date.year, 4);
        result += '-';
        appendPadded(result, date.month, 2);
        result += '-';
        appendPadded(result, date.day, 2);
        return true;
    }
}
