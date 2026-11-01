#include "gran/app/internal/app_state.hpp"
#include "gran/layout/layout.hpp"
#include "gran/ui/style_pass.hpp"

namespace gran::app
{
    namespace
    {
        // Проходов подгонки не больше трёх: окно сходится за один, запас — на вложенные списки и цепочки выражений от размеров
        // (выражение, зависящее от собственного размера, сходится не всегда — предел обрывает цикл)
        // At most three fitting passes: a window converges in one, the margin is for nested lists and chains of size expressions
        // (an expression depending on its own size does not always converge — the limit breaks the loop)
        const int FIT_PASSES = 3;
    }

    /*
    ** Компоновка, затем подгонка окон виртуальных списков под видимую область и выражения от размеров:
    ** новые копии и значения получают стили и место в том же кадре — при прокрутке не мелькают пустые распорки.
    **
    ** A layout, then fitting the virtual list windows to the visible area and the size expressions:
    ** the new copies and values get their styles and place in the same frame — no empty spacers flash while scrolling.
    */
    void layoutFrame(AppWindow& win, Size viewport)
    {
        ui::Node& root = *win.document.root;
        const markup::Theme& theme = markup::defaultTheme();

        layout::layoutTree(root, viewport, win.renderer->measurer());
        for (int pass = 0; pass < FIT_PASSES; ++pass)
        {
            // Оба шага выполняются всегда: выражения с $id.width видят размеры этой компоновки / Both steps always run: the expressions with $id.width see the sizes of this layout
            bool windows = win.shared.lists.updateVirtual();
            bool expressions = win.shared.lists.updateExpressions();
            if (!windows && !expressions)
                break;
            ui::resolveStyles(root, win.windowStyle, theme.kinds, &win.animator);
            layout::layoutTree(root, viewport, win.renderer->measurer());
        }
    }
}
