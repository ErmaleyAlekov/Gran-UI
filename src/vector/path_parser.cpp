#include "gran/vector/internal/vector_internal.hpp"

namespace gran::vector
{
    namespace
    {
        // Латинская буква — возможная команда / A Latin letter is a possible command
        bool isLetter(char symbol) noexcept
        {
            return (symbol >= 'A' && symbol <= 'Z') || (symbol >= 'a' && symbol <= 'z');
        }

        // Заглавная форма буквы команды / The capital form of a command letter
        char upperCommand(char symbol) noexcept
        {
            if (symbol >= 'a' && symbol <= 'z')
                return static_cast<char>(symbol - 'a' + 'A');
            return symbol;
        }
    }

    /*
    ** Буква задаёт команду; числа без буквы повторяют прошлую команду (после M/m — как L/l).
    ** Числа в начале или после Z — ошибка: у них нет команды.
    **
    ** A letter sets the command; numbers without a letter repeat the previous command (after M/m — as L/l).
    ** Numbers at the start or after Z are an error: they have no command.
    */
    Path parsePathData(std::string_view data)
    {
        internal::PathBuilder builder;
        internal::PathScanner scanner{data, 0};
        char command = 0;

        internal::skipSeparators(scanner);
        while (scanner.pos < data.size())
        {
            if (isLetter(data[scanner.pos]))
                command = data[scanner.pos++];
            else if (command == 0 || upperCommand(command) == 'Z')
                internal::throwPathError(scanner);
            internal::runCommand(builder, scanner, upperCommand(command), command != upperCommand(command));
            if (command == 'M')
                command = 'L';
            else if (command == 'm')
                command = 'l';
            internal::skipSeparators(scanner);
        }
        return builder.path;
    }
}
