#include "snapshot.hpp"

#include "gran/binding/list_bindings.hpp"
#include "gran/layout/layout.hpp"
#include "gran/markup/theme.hpp"
#include "gran/ui/style_pass.hpp"

namespace snapshot
{
    /*
    ** Списки строятся из <Data>, затем стили и компоновка; виртуальные списки после компоновки
    ** подгоняют окно записей под видимую область, и проход повторяется (как в приложении).
    ** Привязки нужны только на это время: кадр один, модели больше не меняются; их уничтожение
    ** лишь отписывает их от моделей — построенные узлы остаются в документе.
    **
    ** The lists are built from <Data>, then the styles and the layout; virtual lists fit their record
    ** window to the visible area after the layout, and the pass repeats (as in an application).
    ** The bindings are needed only for this time: there is one frame, the models do not change anymore; destroying them
    ** only unsubscribes them from the models — the built nodes stay in the document.
    */
    void layoutDocument(gran::markup::UiDocument& document, const gran::style::ComputedStyle& window, gran::Size size,
        gran::layout::ContentMeasurer& measurer)
    {
        const gran::markup::Theme& theme = gran::markup::defaultTheme();
        gran::binding::ListBindings lists;

        lists.adoptData(document.data);
        lists.bind(*document.root);
        for (int pass = 0; pass < 4; ++pass)
        {
            gran::ui::resolveStyles(*document.root, window, theme.kinds);
            gran::layout::layoutTree(*document.root, size, measurer);
            if (!lists.updateVirtual())
                break;
        }
    }
}
