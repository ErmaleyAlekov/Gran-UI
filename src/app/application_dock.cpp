#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <algorithm>
#include <string>

namespace gran
{
    namespace
    {
        // Текст в значение атрибута XML / A text into an XML attribute value
        std::string escapeAttribute(const std::string& text)
        {
            std::string out;

            for (char symbol : text)
            {
                if (symbol == '&')
                    out += "&amp;";
                else if (symbol == '<')
                    out += "&lt;";
                else if (symbol == '"')
                    out += "&quot;";
                else
                    out += symbol;
            }
            return out;
        }

        // Узел не DockPanel — ошибка программиста / A node that is not a DockPanel is a programmer error
        void requirePanel(const ui::Node& panel)
        {
            if (panel.kind != ui::ElementKind::DOCK_PANEL)
                throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::APP_WRONG_ELEMENT,
                    {ui::elementName(panel.kind), ui::elementName(ui::ElementKind::DOCK_PANEL)}));
        }
    }

    /*
    ** Показ панели делает её выбранной на крае (как щелчок по заголовку), скрытие отдаёт край соседке. Оторванная
    ** панель сначала возвращается: её окно закрывается (содержимое вернётся в finishWindow). Работает и до run().
    **
    ** Showing a panel makes it the chosen one on the edge (as a click on the title), hiding gives the edge to a neighbor. A torn-off
    ** panel returns first: its window closes (the content returns in finishWindow). It works before run() too.
    */
    void Application::showDockPanel(ui::Node& panel, bool shown)
    {
        app::AppWindow* floatWin = nullptr;

        requirePanel(panel);
        floatWin = app::floatWindowOf(*_state, panel);
        if (floatWin != nullptr && floatWin->document.root)
            closeWindow(*floatWin->document.root);
        panel.content.dock.hidden = !shown;
        if (shown)
            ui::chooseDockPanel(panel);
        else if (panel.parent != nullptr)
            ui::markDirty(*panel.parent, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
    }

    /*
    ** Окно-инструмент с владельцем — окном панели: заголовок — title панели, размер — последняя рамка панели (не меньше
    ** 240 × 160). Содержимое переносится сразу; панель на крае скрыта, пока окно открыто.
    **
    ** A tool window owned by the panel window: the title is the panel title, the size is the last panel frame (not less than
    ** 240 × 160). The content moves at once; the panel at the edge is hidden while the window is open.
    */
    void Application::floatDockPanel(ui::Node& panel)
    {
        WindowOptions options;

        requirePanel(panel);
        if (panel.content.dock.floating || panel.children.empty())
            return;
        float width = std::max(240.0f, panel.layout.frame.width);
        float height = std::max(160.0f, panel.layout.frame.height);
        std::string markup = "<Window kind=\"tool\" position=\"centerOwner\" title=\"" + escapeAttribute(panel.content.text)
            + "\" width=\"" + std::to_string(static_cast<int>(width)) + "\" height=\"" + std::to_string(static_cast<int>(height))
            + "\"><Stack grow=\"1\" padding=\"8\" spacing=\"6\"/></Window>";
        options.owner = &panel;
        ui::Node& root = openWindowString(markup, options);
        app::floatPanel(*_state, panel, app::windowOf(*_state, root));
    }
}

namespace gran::app
{
    // Маршрутизатор окна сообщает о броске в центр — отрывает программа / The window router reports a drop into the center — the program tears it off
    void AppWindow::floatDockPanel(ui::Node& panel)
    {
        if (shared.owner != nullptr)
            shared.owner->floatDockPanel(panel);
    }
}
