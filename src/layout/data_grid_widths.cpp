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

        /*
        ** Новые ширины меняют размер строк при том же доступном месте — кэш измерений по доступному месту
    ** этого не видит. Поэтому строки, заголовок, список и прокрутка забывают свои измерения.
    **
    ** New widths change the row size at the same available room — the measurement cache keyed by the available room
    ** does not see it. So the rows, the header, the list and the scroll forget their measurements.
    */
        void forgetGridLines(ui::Node& grid) noexcept
        {
            ui::Node* rows = ui::gridRows(grid);

            forgetMeasure(ui::gridHeader(grid));
            if (rows == nullptr)
                return;
            forgetMeasure(rows->parent);
            forgetMeasure(rows);
            for (const std::unique_ptr<ui::Node>& row : rows->children)
            {
                if (row->kind == ui::ElementKind::GRID_ROW)
                    forgetMeasure(row.get());
            }
        }
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
