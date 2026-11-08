#include "gran/rhi/adapter_choice.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <cctype>
#include <cstdlib>

namespace gran::rhi
{
    // Переменная читается при каждом создании устройства: тесты и программы могут менять её на ходу.
    // The variable is read on every device creation: tests and programs may change it on the fly.
    std::string requestedAdapter()
    {
        const char* value = std::getenv("GRAN_ADAPTER");

        if (value == nullptr)
            return std::string();
        return std::string(value);
    }

    // Наивный поиск подстроки достаточен: имена адаптеров короче сотни символов, карт — единицы.
    // A naive substring search is enough: adapter names are under a hundred characters, cards are few.
    bool adapterMatches(const std::string& request, const std::string& name)
    {
        if (request.size() > name.size())
            return false;
        for (std::size_t start = 0; start + request.size() <= name.size(); ++start)
        {
            std::size_t index = 0;
            while (index < request.size()
                && std::tolower(static_cast<unsigned char>(name[start + index]))
                    == std::tolower(static_cast<unsigned char>(request[index])))
                ++index;
            if (index == request.size())
                return true;
        }
        return false;
    }

    // Без переменной разрешён любой адаптер / Without the variable any adapter is allowed
    bool adapterAllowed(const std::string& name)
    {
        return adapterMatches(requestedAdapter(), name);
    }

    // Сообщение называет и запрос, и что нашлось — сразу видно опечатку / The message names both the request and what was found — a typo is seen at once
    void adapterNotFound(const std::string& seen)
    {
        std::string names = seen;

        // Список собирается как «имя; имя; » — хвостовой разделитель убирается.
        // The list is gathered as "name; name; " — the trailing separator is removed.
        if (names.size() >= 2 && names.compare(names.size() - 2, 2, "; ") == 0)
            names.resize(names.size() - 2);
        throw GpuException(ErrorCode::GPU, i18n::format(i18n::Message::GPU_ADAPTER_NOT_FOUND,
            {requestedAdapter(), names}), 0);
    }
}
