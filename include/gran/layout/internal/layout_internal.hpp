#ifndef GRAN_LAYOUT_INTERNAL_LAYOUT_INTERNAL_HPP
#define GRAN_LAYOUT_INTERNAL_LAYOUT_INTERNAL_HPP

#include "gran/layout/layout.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace gran::layout::internal
{
    // Состояние одного прохода компоновки / The state of one layout pass
    struct LayoutPass
    {
        ContentMeasurer& measurer;
        std::uint32_t generation;
    };

    // ---- axis.cpp: главная и поперечная оси / the main and cross axes ----

    // Размер по главной оси (ширина у горизонтального стека) / The size along the main axis (the width for a horizontal stack)
    float mainOf(Size size, bool horizontal) noexcept;

    // Размер по поперечной оси / The size along the cross axis
    float crossOf(Size size, bool horizontal) noexcept;

    // Размер из главной и поперечной компонент / A size from the main and cross components
    Size sizeFrom(float main, float cross, bool horizontal) noexcept;

    // Сумма отступов по главной оси (лево + право или верх + низ) / The insets sum along the main axis
    float insetsMain(const Insets& insets, bool horizontal) noexcept;

    // Сумма отступов по поперечной оси / The insets sum along the cross axis
    float insetsCross(const Insets& insets, bool horizontal) noexcept;

    // ---- box_model.cpp: модель блока / the box model ----

    // padding + border: отступ содержимого от рамки / padding + border: the content inset from the frame
    Insets contentInsets(const style::ComputedStyle& style) noexcept;

    // Длина в пикселях; NaN — auto или проценты от неизвестного / A length in pixels; NaN — auto or percent of an unknown
    float resolveLength(const style::Length& length, float reference) noexcept;

    // Ограничение размера минимумом и максимумом (минимум важнее, как в CSS)
    // Clamping a size by the minimum and the maximum (the minimum wins, as in CSS)
    float clampSize(float value, float minimum, float maximum) noexcept;

    // Доступный размер минус отступы (бесконечность остаётся бесконечностью)
    // The available size minus the insets (infinity stays infinity)
    Size shrinkAvailable(Size available, const Insets& insets) noexcept;

    // Размер узла по оси задан явно (не auto) / The node size along the axis is set explicitly (not auto)
    bool hasFixedSize(const ui::Node& node, bool horizontal) noexcept;

    // ---- layout_engine.cpp ----

    // ---- layout_reuse.cpp: локальная перекомпоновка / local relayout ----

    /*
    ** Измерение из кэша. Запись годна, если она сделана в этом проходе или узел с тех пор
    ** не помечался DIRTY_LAYOUT (markDirty поднимает этот флаг ко всем предкам изменённого узла,
    ** поэтому чистый узел — это неизменившееся поддерево).
    **
    ** A measurement from the cache. An entry is valid if it was made in this pass or the node has not been
    ** marked DIRTY_LAYOUT since (markDirty raises this flag to all ancestors of a changed node,
    ** so a clean node is an unchanged subtree).
    */
    bool findCached(const ui::Node& node, Size available, std::uint32_t generation, Size& result) noexcept;

    // Запомнить измерение (два последних размера доступного места) / Remember a measurement (the two last available sizes)
    void storeCache(ui::Node& node, Size available, Size result, std::uint32_t generation) noexcept;

    /*
    ** Чистая ветвь с прежним размером рамки не размещается заново: при том же месте — ничего,
    ** при новом — все рамки поддерева сдвигаются на разницу. true — ветвь обработана.
    **
    ** A clean branch with the former frame size is not arranged anew: at the same place — nothing,
    ** at a new one — all frames of the subtree are shifted by the difference. true — the branch is handled.
    */
    bool reuseArrangement(ui::Node& node, Rect frame) noexcept;

    // Размер рамки узла при доступном размере (с кэшем прохода) / The node frame size for an available size (with the pass cache)
    Size measureNode(ui::Node& node, Size available, LayoutPass& pass);

    // Размещение узла в рамке и его детей / Placing the node in a frame and its children
    void arrangeNode(ui::Node& node, Rect frame, LayoutPass& pass);

    // ---- layout_content.cpp: выбор алгоритма по виду узла / choosing the algorithm by node kind ----

    // Размер содержимого (без padding и border) / The content size (without padding and border)
    Size measureContent(ui::Node& node, Size contentAvailable, LayoutPass& pass);

    // Размещение детей в прямоугольнике содержимого / Placing the children in the content rectangle
    void arrangeContent(ui::Node& node, Rect content, LayoutPass& pass);

    // ---- leaf_layout.cpp ----

    // Собственный размер листа: текст, картинка, поле ввода, флажок, ползунок
    // The intrinsic size of a leaf: text, an image, an input, a check box, a slider
    Size measureLeaf(ui::Node& node, Size contentAvailable, LayoutPass& pass);

    // ---- Stack (flex_*.cpp, stack_*.cpp) ----

    // Элемент гибкой строки: размеры по главной оси — рамка без внешних отступов
    // A flex line item: the main axis sizes are the frame without the margins
    struct FlexItem
    {
        ui::Node* node = nullptr;
        float base = 0.0f;
        float minMain = 0.0f;
        float maxMain = 0.0f;
        float target = 0.0f;
        float cross = 0.0f;
        float marginMain = 0.0f;
        float marginCross = 0.0f;
        bool frozen = false;
    };

    // Видимые дети стека с гипотетическими размерами / The visible stack children with their hypothetical sizes
    void collectFlexItems(ui::Node& stack, Size contentAvailable, std::vector<FlexItem>& items, LayoutPass& pass);

    // Границы строк при переносе: [начало, конец) каждой строки / The line bounds with wrapping: [begin, end) of each line
    std::vector<std::size_t> splitFlexLines(const ui::Node& stack, const std::vector<FlexItem>& items, float available);

    /*
    ** Разрешение гибких длин CSS Flexbox §9.7 для items[first, last): рост по grow или
    ** сжатие по shrink × base с заморозкой нарушивших min/max. available — место
    ** для рамок элементов (уже без промежутков и внешних отступов).
    **
    ** Resolving flexible lengths per CSS Flexbox §9.7 for items[first, last): growing by grow or
    ** shrinking by shrink × base with freezing those violating min/max. available is the space
    ** for the item frames (already without the gaps and the margins).
    */
    void resolveFlexibleLengths(std::vector<FlexItem>& items, std::size_t first, std::size_t last, float available);

    // Поперечные размеры элементов после разрешения главных / The item cross sizes after the main ones are resolved
    void measureFlexCross(std::vector<FlexItem>& items, bool horizontal, float crossAvailable, LayoutPass& pass);

    // Строка гибкого стека: элементы [first, last), смещение и размер по поперечной оси
    // A flex stack line: the items [first, last), the offset and the size along the cross axis
    struct FlexLine
    {
        std::size_t first = 0;
        std::size_t last = 0;
        float crossOffset = 0.0f;
        float crossSize = 0.0f;
    };

    // Размещение строки: justify по главной оси, align/alignSelf и stretch по поперечной (stack_place.cpp)
    // Placing a line: justify along the main axis, align/alignSelf and stretch along the cross axis (stack_place.cpp)
    void placeFlexLine(ui::Node& stack, const std::vector<FlexItem>& items, const FlexLine& line, Rect content,
        LayoutPass& pass);

    // Размер содержимого стека / The stack content size
    Size measureStack(ui::Node& stack, Size contentAvailable, LayoutPass& pass);

    // Размещение детей стека / Placing the stack children
    void arrangeStack(ui::Node& stack, Rect content, LayoutPass& pass);

    // ---- grid_*.cpp ----

    // Элемент сетки: ячейка и охват (уже ограничены числом дорожек) / A grid item: the cell and the span (already bounded by the track count)
    struct GridItem
    {
        ui::Node* node = nullptr;
        std::size_t row = 0;
        std::size_t column = 0;
        std::size_t rowSpan = 1;
        std::size_t columnSpan = 1;
    };

    // Вклад элемента в дорожки: первая дорожка, охват, размер с отступами / An item contribution to tracks: the first track, the span, the size with margins
    struct GridContribution
    {
        std::size_t first = 0;
        std::size_t span = 1;
        float size = 0.0f;
    };

    // Видимые дети сетки с ячейками (grid_items.cpp) / The visible grid children with cells (grid_items.cpp)
    void collectGridItems(ui::Node& grid, std::size_t rows, std::size_t columns, std::vector<GridItem>& items);

    // Дорожка по индексу; за пределами списка — неявная auto / A track by index; beyond the list — an implicit auto
    style::GridTrack trackAt(const std::vector<style::GridTrack>& tracks, std::size_t index) noexcept;

    /*
    ** Доли fr (grid_fractions.cpp): без предела места одна доля — наибольшее «содержимое / fr»,
    ** иначе остаток делится по долям, а дорожка, которой доля меньше минимума, фиксируется.
    **
    ** fr fractions (grid_fractions.cpp): without a space limit one fraction is the largest "content / fr",
    ** otherwise the remainder is shared by fractions, and a track whose share is below its minimum is fixed.
    */
    void resolveFractions(std::vector<float>& sizes, const std::vector<style::GridTrack>& tracks,
        const std::vector<float>& content, float space);

    // Число дорожек: объявленные или одна неявная auto / The track count: the declared ones or one implicit auto
    std::size_t trackCount(const std::vector<style::GridTrack>& tracks) noexcept;

    // Сумма размеров дорожек [first, first + span) с промежутками / The sum of the track sizes [first, first + span) with the gaps
    float spanSize(const std::vector<float>& sizes, std::size_t first, std::size_t span, float gap) noexcept;

    /*
    ** Размеры дорожек (grid_tracks.cpp): фиксированные — по значению, auto — по наибольшему
    ** вкладу элементов с охватом 1, охватывающие элементы растягивают свои auto-дорожки,
    ** доли fr делят остаток (дорожка, которой мало доли, фиксируется на минимуме).
    **
    ** The track sizes (grid_tracks.cpp): fixed ones by value, auto ones by the largest
    ** contribution of items spanning 1, spanning items stretch their auto tracks,
    ** fr fractions share the remainder (a track for which its share is too small is fixed at its minimum).
    */
    std::vector<float> sizeTracks(const std::vector<style::GridTrack>& tracks,
        const std::vector<GridContribution>& contributions, float available, float gap);

    // Размер содержимого сетки / The grid content size
    Size measureGrid(ui::Node& grid, Size contentAvailable, LayoutPass& pass);

    // Размещение детей сетки / Placing the grid children
    void arrangeGrid(ui::Node& grid, Rect content, LayoutPass& pass);

    // ---- canvas_layout.cpp, scroll_layout.cpp ----

    // Канвас: дети по left/top/right/bottom / A canvas: the children by left/top/right/bottom
    Size measureCanvas(ui::Node& canvas, Size contentAvailable, LayoutPass& pass);
    void arrangeCanvas(ui::Node& canvas, Rect content, LayoutPass& pass);

    // Прокрутка: содержимое без ограничения по оси прокрутки / A scroll: the content unbounded along the scroll axis
    Size measureScroll(ui::Node& scroll, Size contentAvailable, LayoutPass& pass);
    void arrangeScroll(ui::Node& scroll, Rect content, LayoutPass& pass);

    // ---- popup_layout.cpp ----

    // Открытые слои Popup поддерева: размер по содержимому, место у якоря в пределах окна / The open Popup layers of the subtree: the size by the content, the place at the anchor within the window
    void layoutPopups(ui::Node& node, Size viewport, LayoutPass& pass);

    // ---- text_area_layout.cpp ----

    // Многострочное поле: ширина — доступная, высота — по тексту (не меньше rows строк)
    // A multi-line input: the width is the available one, the height follows the text (not less than rows lines)
    Size measureTextArea(ui::Node& node, Size contentAvailable, LayoutPass& pass);

    // Размер текста для прокрутки и ограничение смещения / The text size for scrolling and clamping the offset
    void arrangeTextArea(ui::Node& node, Rect content, LayoutPass& pass);
}

#endif
