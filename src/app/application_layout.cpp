#include "gran/app/internal/app_state.hpp"
#include "gran/layout/layout.hpp"
#include "gran/ui/style_pass.hpp"

namespace gran::app
{
    namespace
    {
        // Проходов подгонки не больше трёх: окно сходится за один, запас — на вложенные списки
        // At most three fitting passes: a window converges in one, the margin is for nested lists
        const int FIT_PASSES = 3;
    }

    /*
    ** Компоновка, затем подгонка окон виртуальных списков под видимую область: новые копии
    ** получают стили и место в том же кадре — при прокрутке не мелькают пустые распорки.
    **
    ** A layout, then fitting the virtual list windows to the visible area: the new copies
    ** get their styles and place in the same frame — no empty spacers flash while scrolling.
    */
    void layoutFrame(Application::State& state, Size viewport)
    {
        ui::Node& root = *state.document.root;
        const markup::Theme& theme = markup::defaultTheme();

        layout::layoutTree(root, viewport, state.renderer->measurer());
        for (int pass = 0; pass < FIT_PASSES && state.lists.updateVirtual(); ++pass)
        {
            ui::resolveStyles(root, state.windowStyle, theme.kinds, &state.animator);
            layout::layoutTree(root, viewport, state.renderer->measurer());
        }
    }
}
