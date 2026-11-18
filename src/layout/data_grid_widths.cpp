#include "gran/layout/internal/layout_internal.hpp"
#include "gran/ui/data_grid.hpp"

#include <cmath>

namespace gran::layout::internal
{
    namespace
    {
        /*
        ** Забыть измерение узла этого прохода и пометить к размещению — только сам узел, без подъёма к предкам
        ** (markDirty): предки уже размещаются этим проходом, а флаг у них остался бы до следующего кадра и запускал бы
        ** перекомпоновку в покое (ревью Т-63). Флаг нужен самому узлу: иначе при той же рамке его размещение взялось бы
        ** из прошлого (reuseArrangement), и ячейки остались бы на старых ширинах.
        **
        ** Forget the node measurement of this pass and mark it for placement — only the node itself, without climbing to the ancestors
        ** (markDirty): the ancestors are already being placed by this pass, and the flag would stay on them until the next frame and start
        ** a relayout at rest (review T-63). The node itself needs the flag: otherwise with the same frame its placement would be taken
        ** from the past (reuseArrangement), and the cells would stay at the old widths.
        */
        void forgetMeasure(ui::Node* node) noexcept
        {
            if (node == nullptr)
                return;
            node->layout.cacheCount = 0;
            node->dirty = static_cast<std::uint8_t>(node->dirty | ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        }

        // Ширины одинаковы с точностью до сотой пикселя: дрожь округления не перемеряет строки / The widths are equal to a hundredth of a pixel: rounding jitter does not measure the rows anew
        bool sameWidths(const std::vector<float>& first, const std::vector<float>& second) noexcept
        {
            if (first.size() != second.size())
                return false;
            for (std::size_t index = 0; index < first.size(); ++index)
            {
                if (std::fabs(first[index] - second[index]) > 0.01f)
                    return false;
            }
            return true;
        }
    }

    /*
    ** Новые ширины и сдвиг по горизонтали меняют размер и размещение строк при том же доступном месте — кэш
    ** измерений по доступному месту этого не видит. Поэтому строки, заголовок, списки и прокрутка забывают измерения.
    **
    ** New widths and a horizontal shift change the row size and placement at the same available room — the measurement
    ** cache keyed by the available room does not see it. So the rows, the header, the lists and the scroll forget their measurements.
    */
    void forgetGridLines(ui::Node& grid)
    {
        for (const std::unique_ptr<ui::Node>& child : grid.children)
        {
            if (child->kind == ui::ElementKind::STACK || child->kind == ui::ElementKind::SCROLL)
                forgetMeasure(child.get());
            if (child->kind == ui::ElementKind::SCROLL && !child->children.empty())
                forgetMeasure(child->children.front().get());
        }
        for (ui::Node* line : ui::gridLines(grid))
            forgetMeasure(line);
    }

    /*
    ** Остаток ширины — последнему столбцу, если долевых столбцов нет (как stretchLastSection в Qt): иначе, когда
    ** пользователь сделал единственный столбец «1fr» пиксельным, справа оставалась пустая полоса (ручная проверка Т-64).
    ** Предел maxWidth последнего столбца соблюдается; при бесконечной ширине (таблица в горизонтальной прокрутке) — ничего.
    **
    ** The rest of the width goes to the last column if there are no share columns (as stretchLastSection in Qt): otherwise, when
    ** the user made the only "1fr" column pixel-sized, an empty strip stayed on the right (the manual check T-64).
    ** The maxWidth limit of the last column is kept; at an infinite width (a table in a horizontal scroll) — nothing.
    */
    void fillGridRest(const ui::GridState& state, std::vector<float>& widths, float width)
    {
        float sum = 0.0f;

        if (!std::isfinite(width) || widths.empty() || widths.size() != state.columns.size())
            return;
        for (std::size_t index = 0; index < widths.size(); ++index)
        {
            if (state.columns[index].width.max.kind == style::TrackBoundKind::FRACTION)
                return;
            sum += widths[index];
        }
        if (sum >= width)
            return;
        float& last = widths.back();
        last = std::fmin(last + width - sum, std::fmax(last, state.columns.back().maxWidth));
    }

    // Те же ширины — ничего; новые — запомнить и сбросить измерения строк / The same widths — nothing; new ones — remember them and reset the row measurements
    void replaceGridWidths(ui::Node& grid, std::vector<float> widths)
    {
        ui::GridState& state = *grid.content.grid;

        if (sameWidths(widths, state.widths))
            return;
        state.widths = std::move(widths);
        forgetGridLines(grid);
    }
}
