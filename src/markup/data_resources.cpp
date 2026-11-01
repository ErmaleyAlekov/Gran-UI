#include "gran/markup/internal/build_context.hpp"

#include "beresta.hpp"

#include <utility>

namespace gran::markup::internal
{
    namespace
    {
        // <Variable name type value>: тип по умолчанию — строка; значение тоже не подставляется темой / <Variable name type value>: the default type is a string; the value is not substituted by the theme either
        void readVariable(const beresta::Element& element, BuildContext& context)
        {
            data::Variable variable;
            const std::string* type = findAttribute(element, "type");
            const std::string* value = findAttribute(element, "value");

            variable.name = *findAttribute(element, "name");
            if (type != nullptr && *type == "number")
                variable.type = data::VariableType::NUMBER;
            else if (type != nullptr && *type == "boolean")
                variable.type = data::VariableType::BOOLEAN;
            if (value != nullptr)
                variable.value = *value;
            context.document.variables.push_back(std::move(variable));
        }
    }

    /*
    ** Каждый атрибут <Item> — поле записи как есть: значения темы ($имя) здесь не подставляются,
    ** потому что данные — это текст приложения, а не оформление.
    **
    ** Each <Item> attribute is a record field as it is: theme values ($name) are not substituted here,
    ** because the data is the application text, not the styling.
    */
    void readDataSet(const beresta::Element& data, BuildContext& context)
    {
        DataSet set;

        // <Variable> — тоже данные, а не оформление: читается здесь же / <Variable> is data too, not styling: it is read here as well
        if (data.getLocalName() == "Variable")
        {
            readVariable(data, context);
            return;
        }
        set.name = *findAttribute(data, "name");
        for (const beresta::Element* item = firstElementChild(data); item != nullptr;
             item = nextElementSibling(*item))
        {
            data::Record record;
            for (const beresta::Attribute& attribute : item->getAttributes())
                record[attribute.name] = attribute.value;
            set.records.push_back(std::move(record));
        }
        context.document.data.push_back(std::move(set));
    }
}
