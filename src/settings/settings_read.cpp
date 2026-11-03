#include "gran/settings/internal/settings_xml.hpp"

#include "beresta.hpp"

#include <cstdlib>

namespace gran::generated
{
    extern const unsigned char GRAN_SETTINGS_SCHEMA[];
    extern const std::size_t GRAN_SETTINGS_SCHEMA_SIZE;
}

namespace gran::settings::internal
{
    namespace
    {
        // Атрибут или пустая строка (обязательные проверила схема) / An attribute or an empty string (the schema has checked the required ones)
        std::string attributeOf(const beresta::Element& element, std::string_view name)
        {
            std::optional<std::string> value = element.getAttribute(name);

            if (!value)
                return std::string();
            return *value;
        }

        // <Value>/<Variable>: тип по атрибуту, текст элемента / <Value>/<Variable>: the type by the attribute, the element text
        Value typedValue(const beresta::Element& element)
        {
            Value value;
            std::string type = attributeOf(element, "type");

            value.text = element.getTextContent();
            if (type == "number")
                value.type = ValueType::NUMBER;
            else if (type == "boolean")
                value.type = ValueType::BOOLEAN;
            return value;
        }

        // <List><Item>…</Item></List> / <List><Item>…</Item></List>
        Value listValue(const beresta::Element& element)
        {
            Value value;

            value.type = ValueType::LIST;
            for (const beresta::Node* node = element.getFirstChild(); node != nullptr; node = node->getNextSibling())
            {
                if (node->getType() == beresta::NodeType::ELEMENT)
                    value.items.push_back(node->getTextContent());
            }
            return value;
        }

        // Один элемент верхнего уровня — в модель / One top level element goes into the model
        void readElement(const beresta::Element& element, Settings::Model& model)
        {
            std::string kind(element.getLocalName());

            if (kind == "Window")
                model.windows[attributeOf(element, "layout")] = platform::WindowPlacement{
                    std::atoi(attributeOf(element, "x").c_str()), std::atoi(attributeOf(element, "y").c_str()),
                    std::atoi(attributeOf(element, "width").c_str()), std::atoi(attributeOf(element, "height").c_str()),
                    attributeOf(element, "state") == "maximized"};
            else if (kind == "Attribute")
                model.attributes[AttributeKey{attributeOf(element, "layout"), attributeOf(element, "node"),
                    attributeOf(element, "name")}] = attributeOf(element, "value");
            else if (kind == "Variable")
                model.variables[attributeOf(element, "name")] = typedValue(element);
            else if (kind == "List")
                model.values[attributeOf(element, "name")] = listValue(element);
            else
                model.values[attributeOf(element, "name")] = typedValue(element);
        }
    }

    /*
    ** Разбор со схемой gran-settings.xsd. Любая ошибка XML или схемы — false: испорченный файл
    ** не должен ни ронять программу, ни давать ей полусобранные настройки.
    **
    ** Parsing with the gran-settings.xsd schema. Any XML or schema error is false: a damaged file
    ** must neither crash the program nor give it half-built settings.
    */
    bool readSettings(std::string_view text, Settings::Model& model)
    {
        static const std::shared_ptr<const beresta::xsd::Schema> SCHEMA = beresta::xsd::Schema::fromString(std::string_view(
            reinterpret_cast<const char*>(generated::GRAN_SETTINGS_SCHEMA), generated::GRAN_SETTINGS_SCHEMA_SIZE));
        beresta::CollectingErrorHandler handler;
        beresta::ParserOptions options;
        beresta::Document dom;

        options.schema = SCHEMA;
        options.errorHandler = &handler;
        try
        {
            dom = beresta::XmlParser(options).parse(text);
        }
        catch (const beresta::ParseException&)
        {
            return false;
        }
        if (handler.hasErrors() || dom.getRoot() == nullptr)
            return false;
        for (const beresta::Node* node = dom.getRoot()->getFirstChild(); node != nullptr; node = node->getNextSibling())
        {
            if (node->getType() == beresta::NodeType::ELEMENT)
                readElement(*static_cast<const beresta::Element*>(node), model);
        }
        return true;
    }
}
