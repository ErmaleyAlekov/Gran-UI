#ifndef GRAN_DATA_VARIABLE_HPP
#define GRAN_DATA_VARIABLE_HPP

#include <string>

namespace gran::data
{
    /*
    ** Тип переменной привязки. Тип нужен самому выражению XPath: строка "false" в логическом
    ** контексте истинна (непустая), поэтому флаг объявляется как boolean, счётчик — как number.
    **
    ** The binding variable type. The type is needed by the XPath expression itself: the string "false" is true
    ** in a boolean context (it is not empty), so a flag is declared as boolean and a counter as number.
    */
    enum class VariableType
    {
        STRING,
        NUMBER,
        BOOLEAN
    };

    /*
    ** Переменная привязки ($имя в выражениях {…}): объявляется в <Resources> как <Variable> или
    ** задаётся кодом. Значение хранится текстом: число — по правилам XPath ("2", "0.5"),
    ** логическое — "true" или "false".
    **
    ** A binding variable ($name in {…} expressions): it is declared in <Resources> as <Variable> or
    ** set by code. The value is stored as text: a number — by the XPath rules ("2", "0.5"),
    ** a boolean — "true" or "false".
    */
    struct Variable
    {
        std::string name;
        VariableType type = VariableType::STRING;
        std::string value;
        // persist="true": значение хранится в настройках программы между запусками / persist="true": the value is kept in the program settings between runs
        bool persist = false;
    };
}

#endif
