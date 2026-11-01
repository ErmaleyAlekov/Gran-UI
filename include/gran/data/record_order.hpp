#ifndef GRAN_DATA_RECORD_ORDER_HPP
#define GRAN_DATA_RECORD_ORDER_HPP

#include "gran/data/list_source.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace gran::data
{
    /*
    ** Как сравнивать значения поля: TEXT — текст без учёта регистра (латиница, кириллица, греческий),
    ** NUMBER — числа по правилам XPath (нечисловые — в конце), NATURAL — «файл2» раньше «файл10».
    **
    ** How to compare the field values: TEXT — a text ignoring case (Latin, Cyrillic, Greek),
    ** NUMBER — numbers by the XPath rules (non-numeric ones at the end), NATURAL — "file2" before "file10".
    */
    enum class SortType : std::uint8_t { TEXT, NUMBER, NATURAL };

    // Ключ сортировки: поле, способ сравнения и направление / A sort key: the field, the comparison and the direction
    struct SortKey
    {
        std::string field;
        SortType type = SortType::TEXT;
        bool descending = false;
    };

    using SortKeys = std::vector<SortKey>;

    /*
    ** Фильтр записей (паттерн «Стратегия»): код приложения, обёртка или выражение XPath привязки.
    ** Получает источник и номер, а не только запись: выражению нужен элемент записи в зеркале.
    **
    ** A record filter (the Strategy pattern): the application code, a wrapper or a binding XPath expression.
    ** It gets the source and the index, not just the record: an expression needs the record element in the mirror.
    */
    class RecordFilter
    {
    public:
        virtual ~RecordFilter() = default;

        // Показывать ли запись index источника / Whether to show the record index of the source
        virtual bool accepts(const ListSource& source, std::size_t index) const = 0;
    };

    // ---- sort_keys.cpp ----

    /*
    ** Ключи из текста "client, sum:number desc, file:natural": через запятую «поле[:тип] [asc|desc]».
    ** Ошибка записи — GranException(INVALID_ARGUMENT) с текстом из каталога.
    **
    ** Keys from the text "client, sum:number desc, file:natural": comma separated "field[:type] [asc|desc]".
    ** A writing error is GranException(INVALID_ARGUMENT) with a catalog text.
    */
    SortKeys parseSortKeys(std::string_view text);

    // ---- record_compare.cpp ----

    // Сравнение текстов без учёта регистра (при равенстве — по байтам, чтобы порядок был полным) / Comparing texts ignoring case (when equal — by bytes, so that the order is total)
    int compareText(std::string_view left, std::string_view right);

    // Сравнение чисел; нечисловое значение больше любого числа / Comparing numbers; a non-numeric value is greater than any number
    int compareNumber(std::string_view left, std::string_view right);

    // «Естественное» сравнение: группы цифр — как числа, остальное — как текст / A "natural" comparison: digit groups as numbers, the rest as a text
    int compareNatural(std::string_view left, std::string_view right);

    // Сравнение записей по ключам по очереди (отсутствующее поле — пустая строка) / Comparing records by the keys in turn (a missing field is an empty string)
    int compareRecords(const Record& left, const Record& right, const SortKeys& keys);

    // ---- text_fold.cpp ----

    // Строчные буквы для сравнения без учёта регистра / Lower case letters for a case-insensitive comparison
    std::string foldCase(std::string_view text);
}

#endif
