#include "gran/input/input_router.hpp"

#include <cmath>

namespace gran::input
{
    namespace
    {
        /*
        ** Край, куда упадёт панель: точка в пятой части хоста у края — этот край (слева и справа важнее), иначе
        ** никуда (−1). Отрыв в отдельное окно (DOCK_SIDES) — часть 7б плана.
        **
        ** The edge the panel will drop onto: a point within a fifth of the host at an edge — that edge (left and right matter more), otherwise
        ** nowhere (−1). Tearing off into a separate window (DOCK_SIDES) is part 7b of the plan.
        */
        int dropSideAt(const ui::Node& host, Vec2 point) noexcept
        {
            Vec2 local = ui::windowToNode(host, point);
            const Rect& frame = host.layout.frame;
            float x = (local.x - frame.x) / std::fmax(1.0f, frame.width);
            float y = (local.y - frame.y) / std::fmax(1.0f, frame.height);

            if (x < 0.0f || y < 0.0f || x > 1.0f || y > 1.0f)
                return -1;
            if (x < 0.2f)
                return static_cast<int>(ui::DockSide::LEFT);
            if (x > 0.8f)
                return static_cast<int>(ui::DockSide::RIGHT);
            if (y < 0.2f)
                return static_cast<int>(ui::DockSide::TOP);
            if (y > 0.8f)
                return static_cast<int>(ui::DockSide::BOTTOM);
            return -1;
        }
    }

    /*
    ** Нажатие на заголовок панели: по крестику — скрыть панель, иначе выбрать её на крае и приготовиться к
    ** перетаскиванию (оно начнётся, когда указатель уйдёт дальше 6 пикселей).
    **
    ** A press on a panel title: on the cross — hide the panel, otherwise choose it on the edge and get ready for
    ** dragging (it starts when the pointer moves further than 6 pixels).
    */
    void pressDockTitle(RouterState& state, ui::Node& title, const platform::Event& event)
    {
        ui::Node* panel = ui::dockPanelOf(title);

        if (panel == nullptr)
            return;
        if (closeMarkHit(title, event.position))
        {
            changePressed(state, nullptr);
            setDockHidden(state, *panel, true);
            return;
        }
        ui::chooseDockPanel(*panel);
        state.drag = &title;
        state.dockOrigin = event.position;
        state.dockDragging = false;
    }

    // Перетаскивание заголовка: за порогом — подсветка края под указателем у хоста / Dragging a title: past the threshold — the highlight of the edge under the pointer at the host
    void dragDock(RouterState& state, ui::Node& title, Vec2 point)
    {
        ui::Node* host = nullptr;

        if (title.parent != nullptr)
            host = title.parent->parent;
        if (host == nullptr)
            return;
        if (!state.dockDragging && std::hypot(point.x - state.dockOrigin.x, point.y - state.dockOrigin.y) < 6.0f)
            return;
        state.dockDragging = true;
        int side = dropSideAt(*host, point);
        if (side == host->content.dock.dropSide)
            return;
        host->content.dock.dropSide = side;
        ui::markDirty(*host, ui::DIRTY_PAINT);
    }

    /*
    ** Отпускание заголовка: брошена на край — панель переходит на него и становится там выбранной, хост сообщает
    ** onChange. Подсветка снимается в любом случае.
    **
    ** Releasing a title: dropped onto an edge — the panel moves to it and becomes the chosen one there, the host reports
    ** onChange. The highlight is removed in any case.
    */
    void finishDockDrag(RouterState& state, ui::Node& title)
    {
        ui::Node* panel = ui::dockPanelOf(title);
        ui::Node* host = nullptr;

        if (panel != nullptr)
            host = panel->parent;
        if (host == nullptr)
            return;
        int side = host->content.dock.dropSide;
        host->content.dock.dropSide = -1;
        ui::markDirty(*host, ui::DIRTY_PAINT);
        if (!state.dockDragging || side < 0 || side >= static_cast<int>(ui::DOCK_SIDES))
            return;
        panel->content.dock.side = static_cast<ui::DockSide>(side);
        ui::chooseDockPanel(*panel);
        if (!host->content.onChange.empty())
            state.actions->action(host->content.onChange, *panel);
    }
}
