#include "gran/style/style_sheet.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <vector>

namespace gran::style
{
    namespace
    {
        // Все объявления набора по порядку / All declarations of a set in order
        void applySet(ComputedStyle& style, const PropertySet& set)
        {
            for (const Declaration& declaration : set)
                applyDeclaration(style, declaration);
        }

        // Сохраняем предков для обратного обхода, проверяя цикл двумя указателями.
        // Save ancestors for reverse traversal and detect cycles with two pointers.
        void applyRuleChain(ComputedStyle& style, const StyleRule* rule, int state)
        {
            std::vector<const StyleRule*> children;
            const StyleRule* fast = rule;

            while (rule != nullptr && rule->basedOn != nullptr)
            {
                children.push_back(rule);
                rule = rule->basedOn;
                if (fast != nullptr)
                    fast = fast->basedOn;
                if (fast != nullptr)
                    fast = fast->basedOn;
                if (rule == fast)
                    throw GranException(ErrorCode::RESOURCE, i18n::format(i18n::Message::RESOURCE_STYLE_CYCLE, {rule->name}));
            }
            while (rule != nullptr)
            {
                if (state < 0)
                    applySet(style, rule->block.base);
                else
                    applySet(style, rule->block.states[state]);
                if (children.empty())
                    break;
                rule = children.back();
                children.pop_back();
            }
        }
    }

    // Бит состояния / The state bit
    std::uint16_t stateBit(State state) noexcept
    {
        return static_cast<std::uint16_t>(1u << static_cast<unsigned>(state));
    }

    // Порядок каскада описан в style_sheet.hpp / The cascade order is described in style_sheet.hpp
    ComputedStyle computeStyle(const CascadeInput& input)
    {
        ComputedStyle style;

        if (input.parent != nullptr)
            inheritFrom(style, *input.parent);
        applyRuleChain(style, input.defaults, -1);
        applyRuleChain(style, input.rule, -1);
        if (input.inlineBlock != nullptr)
            applySet(style, input.inlineBlock->base);
        for (std::size_t state = 0; state < STATE_COUNT; ++state)
        {
            if ((input.stateMask & stateBit(static_cast<State>(state))) == 0)
                continue;
            applyRuleChain(style, input.defaults, static_cast<int>(state));
            applyRuleChain(style, input.rule, static_cast<int>(state));
            if (input.inlineBlock != nullptr)
                applySet(style, input.inlineBlock->states[state]);
        }
        return style;
    }
}
