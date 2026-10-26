#include "gran/style/style_sheet.hpp"

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

        /*
        ** Цепочка basedOn от самого дальнего предка к правилу: ближний стиль перекрывает
        ** дальний. state < 0 — основные объявления, иначе — объявления состояния.
        ** Глубина цепочки ограничена: циклы отвергает загрузчик разметки.
        **
        ** The basedOn chain from the farthest ancestor to the rule: a nearer style overrides
        ** a farther one. state < 0 — the base declarations, otherwise the state declarations.
        ** The chain depth is bounded: the markup loader rejects cycles.
        */
        void applyRuleChain(ComputedStyle& style, const StyleRule* rule, int state)
        {
            if (rule == nullptr)
                return;
            applyRuleChain(style, rule->basedOn, state);
            if (state < 0)
                applySet(style, rule->block.base);
            else
                applySet(style, rule->block.states[state]);
        }
    }

    // Бит состояния / The state bit
    std::uint8_t stateBit(State state) noexcept
    {
        return static_cast<std::uint8_t>(1u << static_cast<unsigned>(state));
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
