#include "gran/render/internal/scene_internal.hpp"
#include "gran/ui/data_grid.hpp"

#include <cmath>

namespace gran::render::internal
{
    /*
    ** Строка таблицы (заголовок, копия, закреплённая строка). Ячейки столбцов вне окна прокрутки не рисуются
    ** (виртуализация: их рамки остались от прошлого размещения). Незакреплённые ячейки рисуются первыми и только
    ** правее края закреплённых — уехав под закреплённые, они не просвечивают сквозь их прозрачный фон
    ** (фон выделенной строки виден под всеми ячейками); закреплённые рисуются поверх.
    **
    ** A table line (the header, a copy, a pinned row). The cells of the columns outside the scroll viewport are not painted
    ** (virtualization: their frames are left from the past placement). The non-frozen cells are painted first and only
    ** to the right of the frozen edge — having gone under the frozen ones, they do not show through their transparent background
    ** (the selected row background is visible under all the cells); the frozen ones are painted on top.
    */
    bool paintGridLine(const ui::Node& line, const PaintState& inner, SceneContext& context)
    {
        if ((line.kind != ui::ElementKind::GRID_ROW && line.kind != ui::ElementKind::GRID_HEADER) || ui::gridOf(line) == nullptr)
            return false;
        std::size_t frozen = ui::gridFrozen(line);
        PaintState scrolled = inner;
        const Rect& frame = line.layout.frame;
        if (frozen > 0)
        {
            float edge = ui::gridFrozenEdge(line);
            scrolled = pushClip(Rect{edge, frame.y, std::fmax(0.0f, frame.x + frame.width - edge), frame.height},
                CornerRadii{}, inner, context);
        }
        for (std::size_t index = frozen; index < line.children.size(); ++index)
        {
            if (ui::gridCellShown(line, index))
                paintNode(*line.children[index], scrolled, context);
        }
        for (std::size_t index = 0; index < frozen; ++index)
            paintNode(*line.children[index], inner, context);
        return true;
    }
}
