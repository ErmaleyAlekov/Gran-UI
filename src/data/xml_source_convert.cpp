#include "gran/data/internal/source_readers.hpp"
#include "gran/core/i18n.hpp"

#include "beresta.hpp"

namespace gran::data::internal
{
    namespace
    {
        // Есть ли у элемента дочерние элементы / Whether the element has child elements
        bool hasElements(const beresta::Element& element)
        {
            for (std::size_t index = 0; index < element.getChildCount(); ++index)
            {
                if (element.getChild(index)->getType() == beresta::NodeType::ELEMENT)
                    return true;
            }
            return false;
        }

        /*
        ** Поля без описания Field: атрибуты записи и простые дочерние элементы (только текст) —
        ** <book id="7"><title>…</title></book> даёт поля id и title. Повтор имени берёт первое значение.
        **
        ** The fields without a Field description: the record attributes and simple child elements (text only) —
        ** <book id="7"><title>…</title></book> gives the id and title fields. A repeated name takes the first value.
        */
        void defaultFields(const beresta::Element& element, const SourceSpec& spec, Record& record)
        {
            for (const beresta::Attribute& attribute : element.getAttributes())
                record[attribute.name] = attribute.value;
            for (std::size_t index = 0; index < element.getChildCount(); ++index)
            {
                const beresta::Node* child = element.getChild(index);
                if (child->getType() != beresta::NodeType::ELEMENT)
                    continue;
                const beresta::Element& field = static_cast<const beresta::Element&>(*child);
                if (field.getName() == spec.children || hasElements(field))
                    continue;
                record.emplace(field.getName(), field.getTextContent());
            }
        }
    }

    // Выражение поля, которое не удалось вычислить, даёт пустое поле: одна плохая запись не рвёт загрузку / A field expression that failed to evaluate gives an empty field: one bad record does not break the loading
    TreeItem convertElement(const beresta::Element& element, const SourceSpec& spec, const FieldPaths& fields)
    {
        TreeItem item;

        if (fields.empty())
            defaultFields(element, spec, item.record);
        for (const auto& field : fields)
        {
            std::string& slot = item.record[field.first];
            try
            {
                slot = field.second->evaluate(element).toString();
            }
            catch (const std::exception&)
            {
                slot.clear();
            }
        }
        if (spec.children.empty())
            return item;
        for (const beresta::Element* child : element.findChildElements(spec.children))
            item.children.push_back(convertElement(*child, spec, fields));
        return item;
    }

    // Пути компилируются один раз на файл, а не на каждую запись / The paths are compiled once per file, not per record
    FieldPaths compileFields(const SourceSpec& spec)
    {
        FieldPaths fields;

        for (const FieldSpec& field : spec.fields)
        {
            try
            {
                fields.emplace_back(field.name, std::make_shared<const beresta::xpath::XPathExpression>(field.path));
            }
            catch (const std::exception& error)
            {
                throw GranException(ErrorCode::INVALID_ARGUMENT,
                    i18n::format(i18n::Message::DATA_BAD_FIELD_PATH, {field.path, error.what()}));
            }
        }
        return fields;
    }
}
