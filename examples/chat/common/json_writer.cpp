#include "common/json_value.hpp"

namespace chat
{
    /*
    ** Обязательное по RFC 8259 экранирование: кавычка, обратная косая черта и управляющие
    ** символы U+0000–U+001F (частые — короткой формой, прочие — \u00XX). Байты UTF-8
    ** выше 0x7F пишутся как есть: JSON — это UTF-8.
    **
    ** The escaping required by RFC 8259: the quote, the backslash and the control
    ** characters U+0000–U+001F (the frequent ones in the short form, the others as \u00XX). UTF-8 bytes
    ** above 0x7F are written as is: JSON is UTF-8.
    */
    void writeJsonString(std::string& out, std::string_view text)
    {
        static const char HEX[] = "0123456789abcdef";

        out += '"';
        for (char symbol : text)
        {
            unsigned char code = static_cast<unsigned char>(symbol);
            if (symbol == '"' || symbol == '\\')
                out.append(1, '\\').append(1, symbol);
            else if (symbol == '\n')
                out += "\\n";
            else if (symbol == '\r')
                out += "\\r";
            else if (symbol == '\t')
                out += "\\t";
            else if (code < 0x20)
                out.append("\\u00").append(1, HEX[code >> 4]).append(1, HEX[code & 0x0F]);
            else
                out += symbol;
        }
        out += '"';
    }
}
