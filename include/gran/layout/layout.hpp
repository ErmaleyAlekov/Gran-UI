#ifndef GRAN_LAYOUT_LAYOUT_HPP
#define GRAN_LAYOUT_LAYOUT_HPP

#include "gran/ui/node.hpp"

namespace gran::layout
{
    /*
    ** Измеритель содержимого листьев (паттерн Strategy): текст и картинки меряет
    ** модуль text/ и image/, а тесты компоновки подставляют простую заглушку.
    **
    ** The leaf content measurer (the Strategy pattern): the text/ and image/ modules
    ** measure text and images, while the layout tests substitute a simple stub.
    */
    class ContentMeasurer
    {
    public:
        virtual ~ContentMeasurer() = default;

        // Размер текста узла при ширине строки не больше maxWidth (бесконечность — без переноса)
        // The node text size with a line width not above maxWidth (infinity — no wrapping)
        virtual Size measureText(const ui::Node& node, float maxWidth) = 0;

        // Собственный размер картинки узла (0×0 — ещё не загружена) / The node image intrinsic size (0×0 — not loaded yet)
        virtual Size imageSize(const ui::Node& node) = 0;
    };

    /*
    ** Компоновка дерева: корень занимает всё окно, каждый узел получает layout.frame
    ** (рамка блока в координатах окна). Один проход измерения и один — размещения,
    ** повторные измерения внутри прохода берутся из кэша узла.
    **
    ** Laying out the tree: the root takes the whole window, every node gets layout.frame
    ** (the border box in window coordinates). One measure pass and one arrange pass,
    ** repeated measurements within a pass come from the node cache.
    */
    void layoutTree(ui::Node& root, Size viewport, ContentMeasurer& measurer);
}

#endif
