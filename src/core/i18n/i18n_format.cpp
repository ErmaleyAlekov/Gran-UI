#include "gran/core/i18n.hpp"

namespace gran::i18n
{
    namespace
    {
        /*
        ** Подставляет аргумент по метке {N}, начинающейся с позиции start шаблона.
        ** Возвращает позицию после метки или start, если это не метка.
        **
        ** Substitutes the argument for the {N} placeholder starting at the template position start.
        ** Returns the position after the placeholder, or start if it is not a placeholder.
        */
        std::size_t substitute(std::string_view text, std::size_t start,
            std::initializer_list<std::string_view> arguments, std::string& result)
        {
            std::size_t position = start + 1;
            std::size_t index = 0;

            while (position < text.size() && text[position] >= '0' && text[position] <= '9')
            {
                index = index * 10 + static_cast<std::size_t>(text[position] - '0');
                ++position;
            }
            if (position == start + 1 || position >= text.size() || text[position] != '}')
                return start;
            if (index < arguments.size())
                result += *(arguments.begin() + index);
            return position + 1;
        }
    }

    // Шаблон сообщения в каталоге: линейный поиск по небольшой таблице
    // The message template in a catalog: a linear search over a small table
    const char* findTemplate(const Catalog& catalog, Message id) noexcept
    {
        for (std::size_t index = 0; index < catalog.size; ++index)
        {
            if (catalog.entries[index].id == id)
                return catalog.entries[index].text;
        }
        return nullptr;
    }

    // Текст на текущем языке; без перевода — английский, без него — номер сообщения
    // The text in the current language; without a translation — English, without it — the message number
    std::string format(Message id, std::initializer_list<std::string_view> arguments)
    {
        const char* pattern = findTemplate(currentCatalog(), id);
        std::string result;

        if (pattern == nullptr)
            pattern = findTemplate(ENGLISH_CATALOG, id);
        if (pattern == nullptr)
            return "#" + std::to_string(static_cast<int>(id));
        std::string_view text(pattern);
        for (std::size_t position = 0; position < text.size();)
        {
            std::size_t next = position;
            if (text[position] == '{')
                next = substitute(text, position, arguments, result);
            if (next == position)
            {
                result += text[position];
                next = position + 1;
            }
            position = next;
        }
        return result;
    }
}
