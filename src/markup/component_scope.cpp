#include "gran/markup/internal/components.hpp"

#include "beresta.hpp"

#include <string>

namespace gran::markup::internal
{
    namespace
    {
        // id экземпляра или «Компонент-строка-столбец»: имя без id устойчиво, пока файл не правили / The instance id or "Component-line-column": the name without an id is stable while the file is not edited
        std::string idPrefix(const beresta::Element& instance, const ComponentSpec& spec)
        {
            const std::string* id = findAttribute(instance, "id");
            const beresta::SourceLocation& location = instance.getSourceLocation();

            if (id != nullptr)
                return *id;
            return spec.name + "-" + std::to_string(location.line) + "-" + std::to_string(location.column);
        }

        // Все id тела (и детей вложенных экземпляров) → «приставка.id» / All the body ids (and of the nested instance children) → "prefix.id"
        void collectIds(const beresta::Element& element, const std::string& prefix, Substitution& substitution)
        {
            const std::string* id = findAttribute(element, "id");

            if (id != nullptr)
                substitution.ids[*id] = prefix + "." + *id;
            for (const beresta::Element* child = firstElementChild(element); child != nullptr; child = nextElementSibling(*child))
                collectIds(*child, prefix, substitution);
        }
    }

    /*
    ** Значение параметра — атрибут экземпляра, иначе default; незаданный параметр без default в values
    ** не попадает (атрибут тела «{$имя}» тогда пропадает целиком). Корень тела получает id экземпляра:
    ** ссылки тела на свой корень ($frame.width) ведут к нему.
    **
    ** A parameter value is the instance attribute, otherwise default; an unset parameter without default does not get
    ** into values (a body attribute "{$name}" then disappears whole). The body root gets the instance id:
    ** the body references to its root ($frame.width) lead to it.
    */
    Substitution makeSubstitution(const beresta::Element& instance, const ComponentSpec& spec, const BuildContext& context)
    {
        Substitution substitution;
        const std::string* instanceId = findAttribute(instance, "id");
        const std::string* rootId = findAttribute(*spec.body, "id");

        substitution.spec = &spec;
        substitution.tokens = &context.tokens;
        substitution.styles = &context.document.styles;
        substitution.prefix = idPrefix(instance, spec);
        for (const ComponentParam& param : spec.params)
        {
            const std::string* value = findAttribute(instance, param.name);
            if (value != nullptr)
                substitution.values[param.name] = *value;
            else if (param.defaultValue.has_value())
                substitution.values[param.name] = *param.defaultValue;
        }
        collectIds(*spec.body, substitution.prefix, substitution);
        if (instanceId != nullptr && rootId != nullptr)
            substitution.ids[*rootId] = *instanceId;
        return substitution;
    }
}
