#include "gran/markup/customization.hpp"

#include "beresta.hpp"

#include <sstream>
#include <string>

namespace gran::markup
{
    namespace
    {
        // Сравниваем целые слова списка XSD, разделённые пробелами.
        // Compare complete whitespace-separated words from the XSD list.
        bool containsPermission(const std::string& list, Customization permission)
        {
            std::istringstream words(list);
            std::string word;

            while (words >> word)
            {
                if ((word == "move" && permission == Customization::MOVE)
                    || (word == "hide" && permission == Customization::HIDE)
                    || (word == "style" && permission == Customization::STYLE)
                    || (word == "command" && permission == Customization::COMMAND)
                    || (word == "insert" && permission == Customization::INSERT))
                    return true;
            }
            return false;
        }
    }

    // Первый явный список завершает поиск, даже когда он пуст.
    // The first explicit list ends the search, even when it is empty.
    bool permitsCustomization(const beresta::Element& element, Customization permission)
    {
        for (const beresta::Element* current = &element; current != nullptr;
             current = dynamic_cast<const beresta::Element*>(current->getParent()))
        {
            const auto rule = current->getAttribute("customizable");
            if (rule)
                return containsPermission(*rule, permission);
        }
        return false;
    }
}
