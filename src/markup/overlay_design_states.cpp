#include "gran/markup/internal/overlay.hpp"

namespace gran::markup::internal
{
    namespace
    {
        struct States
        {
            bool limits = false;
            bool contrast = false;
            bool size = false;
            std::vector<std::pair<ui::Node*, std::uint16_t>> masks;
        };

        // Отмечаем только явно объявленные ограничения документа.
        // Record only constraints explicitly declared by the document.
        void noteLimits(const beresta::Element& element, States& states)
        {
            states.contrast = states.contrast || element.hasAttribute("minTextContrast");
            states.size = states.size || element.hasAttribute("minTargetSize");
            states.limits = states.limits || states.contrast || states.size
                || element.hasAttribute("requireAccessibleName");
        }

        // Сохраняем исходные маски перед проверкой вариантов состояний.
        // Save the original masks before checking state variants.
        void collect(ui::Node& node, States& states)
        {
            if (node.domElement) noteLimits(*node.domElement, states);
            states.masks.emplace_back(&node, node.stateMask);
            for (auto& child : node.children) collect(*child, states);
        }

        // Обычное окно без правил пропускает выделение памяти для вариантов состояний.
        // An ordinary window without rules skips allocating state variants.
        bool hasLimits(const ui::Node& node, States& states)
        {
            if (node.domElement) noteLimits(*node.domElement, states);
            if (states.limits) return true;
            for (const auto& child : node.children)
                if (hasLimits(*child, states)) return true;
            return false;
        }
    }

    // После проверки возвращаем реальные состояния и оставляем первый кадр грязным.
    // Restore actual control states after validation and leave the first frame dirty.
    void inspectDesignStates(UiDocument& document, const std::function<void(ui::Node&, bool)>& inspect)
    {
        States states;
        noteLimits(*document.dom->getRoot(), states);
        if (!states.limits && !hasLimits(*document.root, states)) return;
        collect(*document.root, states);
        if (states.contrast || states.size)
        {
            for (std::size_t index = 0; index <= style::STATE_COUNT; ++index)
            {
                const auto bit = index == style::STATE_COUNT ? 0 : style::stateBit(static_cast<style::State>(index));
                bool combined = false;
                for (auto [node, mask] : states.masks)
                {
                    node->stateMask = bit;
                    combined |= (mask | bit) != bit;
                }
                inspect(*document.root, states.size);
                if (states.contrast && bit && combined)
                {
                    for (auto [node, mask] : states.masks) node->stateMask = mask | bit;
                    inspect(*document.root, false);
                }
            }
            for (auto [node, mask] : states.masks)
                node->stateMask = mask;
        }
        inspect(*document.root, states.size);
        for (auto [node, mask] : states.masks)
        {
            node->styled = false;
            node->dirty |= ui::DIRTY_STYLE | ui::DIRTY_LAYOUT | ui::DIRTY_PAINT;
        }
    }
}
