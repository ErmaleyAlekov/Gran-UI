#include "gran/input/input_router.hpp"

#include <algorithm>

namespace gran::input
{
    // Точка окна над крестиком у правого края заголовка (вкладки документов, заголовок панели) / A window point over the cross at the header right edge (document tabs, a panel title)
    bool closeMarkHit(const ui::Node& header, Vec2 point)
    {
        Vec2 local = ui::windowToNode(header, point);
        const Rect& frame = header.layout.frame;
        float right = frame.x + frame.width - header.computed.padding.right - header.computed.borderWidth.right;
        float mark = header.computed.fontSize * ui::TAB_CLOSE_SIZE;

        return local.x >= right - mark && local.x <= frame.x + frame.width;
    }

    // Скрыть или показать панель (крестик, gran:dockHide, gran:dockShow); onChange хоста / Hiding or showing a panel (the cross, gran:dockHide, gran:dockShow); the host onChange
    void setDockHidden(RouterState& state, ui::Node& panel, bool hidden)
    {
        ui::Node* host = panel.parent;

        if (host == nullptr || panel.content.dock.hidden == hidden)
            return;
        panel.content.dock.hidden = hidden;
        if (!hidden)
            ui::chooseDockPanel(panel);
        ui::markDirty(*host, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        if (!host->content.onChange.empty())
            state.actions->action(host->content.onChange, panel);
    }

    /*
    ** Ручка края: длина края — расстояние от края хоста до указателя, не меньше 40 пикселей (сверху место
    ** ограничит компоновка: центру остаётся не меньше 40 пикселей).
    **
    ** The edge handle: the edge length is the distance from the host edge to the pointer, not less than 40 pixels (from above the room
    ** is limited by the layout: the center keeps at least 40 pixels).
    */
    void dragDockHandle(ui::Node& handle, Vec2 point)
    {
        ui::Node* host = handle.parent;

        if (host == nullptr)
            return;
        Vec2 local = ui::windowToNode(*host, point);
        const Rect& frame = host->layout.frame;
        ui::DockSide side = handle.content.dock.side;
        float length = local.x - frame.x;
        if (side == ui::DockSide::RIGHT)
            length = frame.x + frame.width - local.x;
        if (side == ui::DockSide::TOP)
            length = local.y - frame.y;
        if (side == ui::DockSide::BOTTOM)
            length = frame.y + frame.height - local.y;
        host->content.dock.sizes[static_cast<std::size_t>(side)] = std::max(40.0f, length);
        ui::markDirty(*host, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
    }

    // Клавиатура на заголовке: Enter и пробел выбирают панель на крае / The keyboard on a title: Enter and space choose the panel on the edge
    void activateDockTitle(ui::Node& title)
    {
        ui::Node* panel = ui::dockPanelOf(title);

        if (panel != nullptr)
            ui::chooseDockPanel(*panel);
    }
}
