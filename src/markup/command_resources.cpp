#include "gran/markup/internal/build_context.hpp"
#include "gran/core/i18n.hpp"
#include "gran/style/value_parsers.hpp"

#include "beresta.hpp"

#include <utility>

namespace gran::markup::internal
{
    namespace
    {
        // Значение атрибута или пусто / The attribute value or empty
        std::string attributeText(const beresta::Element& element, std::string_view name)
        {
            const std::string* value = findAttribute(element, name);

            if (value == nullptr)
                return std::string();
            return *value;
        }

        // Область сочетания; значения проверила схема / The shortcut scope; the schema has checked the values
        ui::CommandScope readScope(const beresta::Element& element)
        {
            static const style::Keyword SCOPES[] = {{"window", 0}, {"focus", 1}, {"global", 2}};
            const std::string* value = findAttribute(element, "scope");

            if (value == nullptr)
                return ui::CommandScope::WINDOW;
            return static_cast<ui::CommandScope>(style::parseKeyword(*value, SCOPES));
        }
    }

    /*
    ** Схема проверила вид записи сочетаний (шаблон), здесь — имена клавиш и повтор модификаторов:
    ** "Mod+Foo" или "Ctrl+Ctrl+S" дают ошибку с позицией атрибута key.
    **
    ** The schema has checked the shortcut notation (a pattern), here — the key names and repeated modifiers:
    ** "Mod+Foo" or "Ctrl+Ctrl+S" give an error with the key attribute position.
    */
    void readCommand(const beresta::Element& command, BuildContext& context)
    {
        ui::Command result;
        const std::string* enabled = findAttribute(command, "enabled");
        std::string keys = attributeText(command, "key");

        result.id = attributeText(command, "id");
        result.text = attributeText(command, "text");
        result.action = attributeText(command, "action");
        result.scope = readScope(command);
        result.within = attributeText(command, "within");
        if (enabled != nullptr)
            result.enabled = style::parseBoolean(*enabled);
        if (!ui::parseShortcuts(keys, result.keys))
            throwAttributeError(command, "key", i18n::format(i18n::Message::MARKUP_BAD_SHORTCUT, {keys}));
        context.document.commands.push_back(std::move(result));
    }
}
