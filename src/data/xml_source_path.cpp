#include "gran/data/internal/source_readers.hpp"
#include "gran/core/i18n.hpp"

namespace gran::data::internal
{
    namespace
    {
        // Имя шага подходит под элемент: * — любой, имя без префикса сравнивается и с локальной частью / A step name fits an element: * — any, a name without a prefix is compared with the local part too
        bool nameFits(const std::string& step, const std::string& element)
        {
            std::size_t colon = element.find(':');

            if (step == "*" || step == element)
                return true;
            return step.find(':') == std::string::npos && colon != std::string::npos
                && element.compare(colon + 1, std::string::npos, step) == 0;
        }

        // Шаги начиная с step против открытых элементов начиная с depth (с возвратом для «//») / The steps from step against the open elements from depth (with backtracking for "//")
        bool matchFrom(const std::vector<XmlStep>& steps, std::size_t step, const std::vector<std::string>& stack,
            std::size_t depth)
        {
            if (step == steps.size())
                return depth == stack.size();
            if (!steps[step].descendant)
                return depth < stack.size() && nameFits(steps[step].name, stack[depth])
                    && matchFrom(steps, step + 1, stack, depth + 1);
            for (std::size_t place = depth; place < stack.size(); ++place)
            {
                if (nameFits(steps[step].name, stack[place]) && matchFrom(steps, step + 1, stack, place + 1))
                    return true;
            }
            return false;
        }

        // Символы, которых нет в поддерживаемом подмножестве (предикаты, оси, функции, атрибуты) / Characters outside the supported subset (predicates, axes, functions, attributes)
        bool plainName(const std::string& name)
        {
            return !name.empty() && name.find_first_of("[]()@=' \"\t|") == std::string::npos && name.find("::") == std::string::npos;
        }
    }

    /*
    ** Подмножество XPath, которое проверяется на лету событиями SAX: шаги «/имя», «//имя» и «*».
    ** Путь без «/» в начале ищется на любой глубине (как «//»). Сложные условия — в путях полей.
    **
    ** The XPath subset that is checked on the fly by the SAX events: the "/name", "//name" and "*" steps.
    ** A path without a leading "/" is looked for at any depth (as "//"). Complex conditions go into the field paths.
    */
    std::vector<XmlStep> parseXmlPath(const std::string& select)
    {
        std::string path = select;
        std::vector<XmlStep> steps;
        std::size_t position = 0;

        if (path.empty())
            path = "/*/*";
        else if (path[0] != '/')
            path = "//" + path;
        while (position < path.size())
        {
            XmlStep step;
            step.descendant = path.compare(position, 2, "//") == 0;
            position += 1 + static_cast<std::size_t>(step.descendant);
            std::size_t end = path.find('/', position);
            if (end == std::string::npos)
                end = path.size();
            step.name = path.substr(position, end - position);
            if (!plainName(step.name))
                throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::DATA_BAD_SELECT, {select}));
            steps.push_back(std::move(step));
            position = end;
        }
        return steps;
    }

    bool matchXmlPath(const std::vector<XmlStep>& steps, const std::vector<std::string>& stack)
    {
        return matchFrom(steps, 0, stack, 0);
    }
}
