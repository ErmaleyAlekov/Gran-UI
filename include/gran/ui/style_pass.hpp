#ifndef GRAN_UI_STYLE_PASS_HPP
#define GRAN_UI_STYLE_PASS_HPP

#include "gran/ui/node.hpp"

namespace gran::ui
{
    // Число видов элементов / The element kind count
    constexpr std::size_t ELEMENT_KIND_COUNT = 15;

    // Встроенные стили видов элементов (индекс — ElementKind); nullptr — без стиля
    // The built-in element kind styles (the index is ElementKind); nullptr — no style
    struct KindStyles
    {
        const style::StyleRule* rules[ELEMENT_KIND_COUNT] = {};
    };

    /*
    ** Наблюдатель пересчёта стиля (паттерн Observer): аниматор подменяет в next значения,
    ** которые должны не прыгнуть, а плавно перейти от показанных (shown). Первый расчёт
    ** стиля узла наблюдателю не сообщается — при загрузке переходы не играют (как в CSS).
    **
    ** A style recompute observer (the Observer pattern): the animator replaces in next the values
    ** that must not jump but move smoothly from the shown ones. The first style computation
    ** of a node is not reported — transitions do not play on load (as in CSS).
    */
    class StyleObserver
    {
    public:
        virtual ~StyleObserver() = default;

        // Стиль узла пересчитан: shown — показанный сейчас, next — новый целевой / The node style is recomputed: shown is the one shown now, next is the new target
        virtual void styleChanged(Node& node, const style::ComputedStyle& shown, style::ComputedStyle& next) = 0;
    };

    /*
    ** Проход стилей: пересчитывает узлы с DIRTY_STYLE и всё их поддерево (наследование),
    ** спускаясь только в ветви с DIRTY_DESCENDANT. Узел, чья смена стиля влияет на размеры,
    ** получает DIRTY_LAYOUT; любой пересчитанный — DIRTY_PAINT.
    **
    ** The style pass: recomputes the nodes with DIRTY_STYLE and their whole subtree (inheritance),
    ** descending only into branches with DIRTY_DESCENDANT. A node whose style change affects sizes
    ** gets DIRTY_LAYOUT; every recomputed one gets DIRTY_PAINT.
    */
    void resolveStyles(Node& root, const style::ComputedStyle& windowStyle, const KindStyles& defaults,
        StyleObserver* observer = nullptr);
}

#endif
