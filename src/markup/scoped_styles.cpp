#include "gran/markup/internal/overlay.hpp"

namespace gran::markup::internal
{
    namespace
    {
        struct ScopedStyles
        {
            beresta::Element* resources = nullptr;
            std::map<std::string, const beresta::Element*> styles;
            std::map<std::pair<std::string, TokenMap>, std::string> copies;
            std::size_t serial = 0;
        };

        // Родительский стиль уже скопирован к моменту создания потомка.
        // The base style already has an alias when its descendant is copied.
        std::string appendStyle(ScopedStyles& sheet, const beresta::Element& source,
            const TokenMap& values, const std::string& baseAlias)
        {
            auto clone = std::make_unique<beresta::Element>(source);
            if (clone->hasAttribute("basedOn"))
                clone->setAttribute("basedOn", baseAlias);
            replaceScopedProperties(*clone, values);
            for (auto* state = firstElementChild(*clone); state; state = nextElementSibling(*state))
                replaceScopedProperties(const_cast<beresta::Element&>(*state), values);
            std::string alias;
            do { alias = "gran-local-" + std::to_string(++sheet.serial); } while (sheet.styles.count(alias));
            clone->setAttribute("name", alias);
            sheet.resources->insertChild(source.getIndexInParent(), std::move(clone));
            sheet.copies.emplace(std::make_pair(*findAttribute(source, "name"), values), alias);
            return alias;
        }

        // Идём вверх по проверенной цепочке, затем создаём копии от основания.
        // Walk the validated chain upwards, then create copies starting at its base.
        std::string copyStyle(ScopedStyles& sheet, std::string name, const TokenMap& values)
        {
            std::vector<const beresta::Element*> pending;
            while (const auto* source = sheet.styles.count(name) ? sheet.styles.at(name) : nullptr)
            {
                const auto known = sheet.copies.find({name, values});
                if (known != sheet.copies.end())
                {
                    name = known->second;
                    break;
                }
                pending.push_back(source);
                const auto base = source->getAttribute("basedOn");
                if (!base) break;
                name = *base;
            }
            for (auto it = pending.rbegin(); it != pending.rend(); ++it)
                name = appendStyle(sheet, **it, values, name);
            return name;
        }

        // При вложенных областях ближайшее значение токена перекрывает внешнее.
        // The nearest token value overrides the outer value in nested regions.
        void walk(beresta::Element& element, const OverlayContext& context, ScopedStyles& sheet, const TokenMap& inherited)
        {
            TokenMap local;
            const auto scope = context.scoped.find(&element);
            if (scope != context.scoped.end())
            {
                local = inherited;
                for (const auto& [name, value] : scope->second)
                    local[name] = value;
            }
            const auto& values = scope == context.scoped.end() ? inherited : local;
            if (!values.empty())
            {
                replaceScopedProperties(element, values);
                if (auto name = element.getAttribute("style"))
                    element.setAttribute("style", copyStyle(sheet, *name, values));
            }
            for (auto* child = firstElementChild(element); child; child = nextElementSibling(*child))
                walk(const_cast<beresta::Element&>(*child), context, sheet, values);
        }
    }

    // Копирование деклараций откладывается до применения всех операций профиля.
    // Defer copying declarations until all profile operations have been applied.
    void materializeTokens(OverlayContext& context)
    {
        if (context.scoped.empty())
            return;
        auto* resources = firstElementChild(*context.candidate.dom->getRoot());
        ScopedStyles sheet;
        sheet.resources = const_cast<beresta::Element*>(resources);
        for (auto* item = firstElementChild(*resources); item; item = nextElementSibling(*item))
            if (item->getLocalName() == "Style")
                sheet.styles.emplace(*findAttribute(*item, "name"), item);
        auto* root = nextElementSibling(*resources);
        walk(const_cast<beresta::Element&>(*root), context, sheet, {});
    }
}
