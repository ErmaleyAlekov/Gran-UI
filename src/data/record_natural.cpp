#include "gran/data/record_order.hpp"

#include <algorithm>

namespace gran::data
{
    namespace
    {
        // Цифра ASCII / An ASCII digit
        bool digit(char symbol) noexcept
        {
            return symbol >= '0' && symbol <= '9';
        }

        /*
        ** Сравнить группы цифр с позиций left[i] и right[j] как числа любой длины: без ведущих нулей
        ** длиннее — больше, при равной длине — по цифрам. Позиции сдвигаются за группы.
        **
        ** Compare the digit groups at left[i] and right[j] as numbers of any length: without leading zeros
        ** the longer one is greater, with an equal length — by the digits. The positions move past the groups.
        */
        int compareDigits(std::string_view left, std::size_t& i, std::string_view right, std::size_t& j)
        {
            std::size_t leftStart = i;
            std::size_t rightStart = j;

            while (i < left.size() && digit(left[i]))
                ++i;
            while (j < right.size() && digit(right[j]))
                ++j;
            std::string_view a = left.substr(leftStart, i - leftStart);
            std::string_view b = right.substr(rightStart, j - rightStart);
            a.remove_prefix(std::min(a.find_first_not_of('0'), a.size()));
            b.remove_prefix(std::min(b.find_first_not_of('0'), b.size()));
            if (a.size() != b.size())
                return (a.size() > b.size()) - (a.size() < b.size());
            int order = a.compare(b);
            return (order > 0) - (order < 0);
        }
    }

    /*
    ** Обе строки сначала сворачиваются в строчные; байты вне цифр сравниваются как коды символов UTF-8.
    ** Равные по смыслу («a01» и «a1») различает полное текстовое сравнение — порядок остаётся полным.
    **
    ** Both strings are folded to lower case first; the bytes outside digits are compared as UTF-8 character codes.
    ** Equal in meaning ("a01" and "a1") are told apart by the full text comparison — the order stays total.
    */
    int compareNatural(std::string_view left, std::string_view right)
    {
        std::string a = foldCase(left);
        std::string b = foldCase(right);
        std::size_t i = 0;
        std::size_t j = 0;

        while (i < a.size() && j < b.size())
        {
            if (digit(a[i]) && digit(b[j]))
            {
                int order = compareDigits(a, i, b, j);
                if (order != 0)
                    return order;
                continue;
            }
            unsigned char x = static_cast<unsigned char>(a[i++]);
            unsigned char y = static_cast<unsigned char>(b[j++]);
            if (x != y)
                return (x > y) - (x < y);
        }
        if (i < a.size() || j < b.size())
            return (i < a.size()) - (j < b.size());
        return compareText(left, right);
    }
}
