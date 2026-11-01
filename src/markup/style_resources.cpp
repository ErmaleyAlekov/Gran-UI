#include "gran/markup/internal/build_context.hpp"
#include "gran/core/i18n.hpp"
#include "gran/style/value_parsers.hpp"

#include "beresta.hpp"

#include <utility>
#include <vector>

namespace gran::markup::internal
{
    namespace
    {
        // Стиль и его элемент (для позиции ошибок) / A style and its element (for error positions)
        using StyleSource = std::pair<style::StyleRule*, const beresta::Element*>;

        // <Theme><Token name value/>...</Theme>; уникальность имён проверила схема
        // <Theme><Token name value/>...</Theme>; the schema has checked the name uniqueness
        void readTheme(const beresta::Element& theme, BuildContext& context)
        {
            for (const beresta::Element* token = firstElementChild(theme); token != nullptr;
                 token = nextElementSibling(*token))
                context.tokens[*findAttribute(*token, "name")] = *findAttribute(*token, "value");
        }

        // <Font family source [weight] [italic]/> / <Font family source [weight] [italic]/>
        void readFont(const beresta::Element& font, BuildContext& context)
        {
            FontFace face;
            const std::string* weight = findAttribute(font, "weight");
            const std::string* italic = findAttribute(font, "italic");

            face.family = *findAttribute(font, "family");
            face.source = *findAttribute(font, "source");
            if (weight != nullptr && *weight == "bold")
                face.weight = 700;
            else if (weight != nullptr && *weight != "normal")
                face.weight = static_cast<int>(style::parseNumber(*weight));
            if (italic != nullptr)
                face.italic = style::parseBoolean(*italic);
            context.document.fonts.push_back(face);
        }

        /*
        ** Связывание basedOn после чтения всех стилей (ссылка может идти вперёд). Цикл a → b → a
        ** схема не ловит (её утверждение запрещает только a → a): цепочка длиннее числа стилей — цикл.
        **
        ** Linking basedOn after all styles are read (a reference may point forward). The schema does not catch
        ** an a → b → a cycle (its assertion forbids only a → a): a chain longer than the style count is a cycle.
        */
        void linkStyles(const std::vector<StyleSource>& sources, BuildContext& context)
        {
            for (const StyleSource& source : sources)
            {
                const std::string* basedOn = findAttribute(*source.second, "basedOn");
                if (basedOn != nullptr)
                    source.first->basedOn = context.document.styles.find(*basedOn);
            }
            for (const StyleSource& source : sources)
            {
                std::size_t steps = 0;
                for (const style::StyleRule* rule = source.first; rule != nullptr; rule = rule->basedOn)
                {
                    if (++steps > sources.size())
                        throwAttributeError(*source.second, "basedOn",
                            i18n::format(i18n::Message::RESOURCE_STYLE_CYCLE, {source.first->name}));
                }
            }
        }
    }

    // Тема, шрифты, стили, данные, переменные и команды в порядке схемы; <xi:include> читает loadComponents / The theme, fonts, styles, data, variables and commands in the schema order; loadComponents reads <xi:include>
    void readResources(const beresta::Element& resources, BuildContext& context)
    {
        std::vector<StyleSource> styles;

        for (const beresta::Element* child = firstElementChild(resources); child != nullptr;
             child = nextElementSibling(*child))
        {
            std::string_view name = child->getLocalName();
            if (name == "Theme")
                readTheme(*child, context);
            else if (name == "Font")
                readFont(*child, context);
            else if (name == "Data" || name == "Variable" || name == "Tree" || name == "TreeList" || name == "SortFilter"
                || name == "Selection")
                readDataSet(*child, context);
            else if (name == "Keyframes")
                readKeyframes(*child, context);
            else if (name == "Command")
                readCommand(*child, context);
            else if (name == "Style")
            {
                style::StyleRule& rule = context.document.styles.add(*findAttribute(*child, "name"));
                readDeclarations(*child, rule.block.base, context);
                readStates(*child, rule.block, context);
                styles.emplace_back(&rule, child);
            }
        }
        linkStyles(styles, context);
    }
}
