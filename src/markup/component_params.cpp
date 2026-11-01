#include "gran/markup/internal/components.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include "beresta.hpp"

#include <sstream>

namespace gran::markup::internal
{
    namespace
    {
        /*
        ** Атрибуты экземпляра, которые переходят на корень тела. Совпадают с атрибутами хозяина
        ** сгенерированной схемы (component_schema.cpp: id, visible, enabled и группа BoxAttributes).
        **
        ** The instance attributes that move onto the body root. They match the host attributes
        ** of the generated schema (component_schema.cpp: id, visible, enabled and the BoxAttributes group).
        */
        const char* const HOST_ATTRIBUTES[] = {
            "id", "visible", "enabled", "width", "height", "minWidth", "maxWidth", "minHeight", "maxHeight",
            "margin", "padding", "grow", "shrink", "alignSelf", "gridRow", "gridColumn", "gridRowSpan",
            "gridColumnSpan", "left", "top", "right", "bottom", "zIndex"};

        // Имена, которые тоже нельзя дать параметру: слот экземпляра, его стиль и имена привязок / The names a parameter cannot take either: the instance slot, its style and the binding names
        const char* const RESERVED_NAMES[] = {"slot", "style", "index", "self", "parent"};

        // Тип по имени из схемы (ParamKind; без атрибута — string) / A type by the schema name (ParamKind; without the attribute — string)
        ParamType parseType(const std::string* name) noexcept
        {
            static const char* const NAMES[] = {"string", "number", "boolean", "color", "paint", "length", "size", "action"};

            for (std::size_t index = 0; name != nullptr && index < sizeof(NAMES) / sizeof(NAMES[0]); ++index)
            {
                if (*name == NAMES[index])
                    return static_cast<ParamType>(index);
            }
            return ParamType::STRING;
        }

        // Имя занято атрибутом экземпляра или привязкой / The name is taken by an instance attribute or by the binding
        bool isReserved(std::string_view name) noexcept
        {
            if (isHostAttribute(name))
                return true;
            for (const char* reserved : RESERVED_NAMES)
            {
                if (name == reserved)
                    return true;
            }
            return false;
        }
    }

    // Лексику значений (тип, values, default у boolean) проверила схема / The value lexis (the type, values, default of a boolean) is checked by the schema
    ComponentParam readParam(const beresta::Element& param, const std::string& component)
    {
        ComponentParam result;
        const std::string* required = findAttribute(param, "required");
        const std::string* values = findAttribute(param, "values");
        std::string value;

        result.name = *findAttribute(param, "name");
        if (isReserved(result.name))
            throwAttributeError(param, "name", i18n::format(i18n::Message::MARKUP_PARAM_RESERVED, {result.name, component}));
        result.type = parseType(findAttribute(param, "type"));
        result.required = required != nullptr && (*required == "true" || *required == "1");
        result.defaultValue = param.getAttribute("default");
        if (values == nullptr)
            return result;
        std::istringstream words(*values);
        while (words >> value)
            result.values.push_back(value);
        return result;
    }

    bool isHostAttribute(std::string_view name) noexcept
    {
        for (const char* host : HOST_ATTRIBUTES)
        {
            if (name == host)
                return true;
        }
        return false;
    }

    // Стиль окна под своим именем остаётся видимым и в теле (как глобальная переменная) / A window style under its own name stays visible in the body too (as a global variable)
    std::string resolveStyleName(std::string_view name, const StyleNames& names, const style::StyleSheet& styles)
    {
        StyleNames::const_iterator found = names.find(name);

        if (found != names.end())
            return found->second;
        if (styles.find(name) == nullptr)
            throw GranException(ErrorCode::MARKUP, i18n::format(i18n::Message::MARKUP_UNKNOWN_STYLE, {name}));
        return std::string(name);
    }
}
