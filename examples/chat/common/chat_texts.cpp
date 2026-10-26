#include "common/chat_texts.hpp"

#include "gran/core/i18n.hpp"

#include <cstring>
#include <vector>

namespace chat
{
    namespace
    {
        // Каталог текущего языка движка; неизвестный язык — английский / The catalog of the current engine language; an unknown language — English
        const ChatCatalog& currentCatalog() noexcept
        {
            if (std::strcmp(gran::i18n::language(), CHAT_RUSSIAN.code) == 0)
                return CHAT_RUSSIAN;
            return CHAT_ENGLISH;
        }

        /*
        ** Метка {N} заменяется N-м аргументом; метка без аргумента и любые другие скобки
        ** остаются как есть (текст ошибки не должен ломаться из-за опечатки в каталоге).
        **
        ** A {N} placeholder is replaced by the N-th argument; a placeholder without an argument and any other braces
        ** stay as they are (an error text must not break because of a typo in the catalog).
        */
        std::string substitute(const char* pattern, const std::vector<std::string_view>& arguments)
        {
            std::string result;

            for (const char* cursor = pattern; *cursor != '\0'; ++cursor)
            {
                bool placeholder = cursor[0] == '{' && cursor[1] >= '0' && cursor[1] <= '9' && cursor[2] == '}';
                std::size_t index = 0;
                if (placeholder)
                    index = static_cast<std::size_t>(cursor[1] - '0');
                if (placeholder && index < arguments.size())
                {
                    result.append(arguments[index]);
                    cursor += 2;
                }
                else
                    result += *cursor;
            }
            return result;
        }
    }

    // Линейный поиск: каталог мал, порядок строк в нём не важен / A linear search: the catalog is small, the row order does not matter
    const char* findChatText(const ChatCatalog& catalog, ChatText id) noexcept
    {
        for (std::size_t index = 0; index < catalog.size; ++index)
        {
            if (catalog.entries[index].id == id)
                return catalog.entries[index].text;
        }
        return nullptr;
    }

    // Текущий язык, затем английский, затем пустой шаблон / The current language, then English, then an empty template
    std::string say(ChatText id, std::initializer_list<std::string_view> arguments)
    {
        const char* pattern = findChatText(currentCatalog(), id);

        if (pattern == nullptr)
            pattern = findChatText(CHAT_ENGLISH, id);
        if (pattern == nullptr)
            pattern = "";
        return substitute(pattern, std::vector<std::string_view>(arguments));
    }
}
