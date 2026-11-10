#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran
{
    /*
    ** Показ панели делает её выбранной на крае (как щелчок по заголовку), скрытие отдаёт край соседке. Работает и
    ** до run(): компоновка первого кадра уже видит новое состояние. Узел не DockPanel — ошибка программиста.
    **
    ** Showing a panel makes it the chosen one on the edge (as a click on the title), hiding gives the edge to a neighbor. It works
    ** before run() too: the first frame layout already sees the new state. A node that is not a DockPanel is a programmer error.
    */
    void Application::showDockPanel(ui::Node& panel, bool shown)
    {
        if (panel.kind != ui::ElementKind::DOCK_PANEL)
            throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::APP_WRONG_ELEMENT,
                {ui::elementName(panel.kind), ui::elementName(ui::ElementKind::DOCK_PANEL)}));
        panel.content.dock.hidden = !shown;
        if (shown)
            ui::chooseDockPanel(panel);
        else if (panel.parent != nullptr)
            ui::markDirty(*panel.parent, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
    }
}
