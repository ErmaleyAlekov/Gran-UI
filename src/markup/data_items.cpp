#include "gran/markup/internal/build_context.hpp"

#include "beresta.hpp"

namespace gran::markup::internal
{
    namespace
    {
        // Тип столбца по слову схемы (по умолчанию text) / The column type by the schema word (text by default)
        data::ColumnType columnType(const std::string* word) noexcept
        {
            if (word == nullptr)
                return data::ColumnType::TEXT;
            if (*word == "number")
                return data::ColumnType::NUMBER;
            if (*word == "date")
                return data::ColumnType::DATE;
            if (*word == "boolean")
                return data::ColumnType::BOOLEAN;
            return data::ColumnType::TEXT;
        }
    }

    // Каждый атрибут <Item> — поле записи как есть / Each <Item> attribute is a record field as it is
    data::Record readRecord(const beresta::Element& item)
    {
        data::Record record;

        for (const beresta::Attribute& attribute : item.getAttributes())
            record[attribute.name] = attribute.value;
        return record;
    }

    std::vector<data::TreeItem> readNodes(const beresta::Element& parent)
    {
        std::vector<data::TreeItem> nodes;

        for (const beresta::Element* item = firstElementChild(parent); item != nullptr; item = nextElementSibling(*item))
        {
            if (item->getLocalName() != "Item")
                continue;
            data::TreeItem node;
            node.record = readRecord(*item);
            node.children = readNodes(*item);
            nodes.push_back(std::move(node));
        }
        return nodes;
    }

    // Заголовок по умолчанию — имя поля / The default title is the field name
    data::ColumnSet readColumns(const beresta::Element& parent)
    {
        data::ColumnSet columns;

        for (const beresta::Element* child = firstElementChild(parent); child != nullptr; child = nextElementSibling(*child))
        {
            if (child->getLocalName() != "Column")
                continue;
            data::Column column;
            column.name = *findAttribute(*child, "name");
            column.title = column.name;
            if (const std::string* title = findAttribute(*child, "title"))
                column.title = *title;
            column.type = columnType(findAttribute(*child, "type"));
            columns.push_back(std::move(column));
        }
        return columns;
    }
}
