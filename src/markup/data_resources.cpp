#include "gran/markup/internal/build_context.hpp"

#include "beresta.hpp"

#include <utility>

namespace gran::markup::internal
{
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
