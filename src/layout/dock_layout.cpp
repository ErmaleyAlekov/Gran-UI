#include "gran/layout/internal/layout_internal.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace gran::layout::internal
{
    namespace
    {
        /*
        ** Видимые панели края по порядку детей: не скрытые (hidden), не оторванные и не скрытые приложением.
        ** Заголовки полосы края видны у них же, отмечен (Checked) — у выбранной; выбранная — первая с active.
        **
        ** The visible edge panels in the children order: not hidden (hidden), not torn off and not hidden by the application.
        ** The edge strip titles are shown for them, the marked one (Checked) is the chosen one; the chosen one is the first with active.
        */
        ui::Node* sidePanels(ui::Node& host, ui::DockSide side, std::vector<ui::Node*>& panels)
        {
            ui::Node* strip = ui::dockStrip(host, side);
            ui::Node* chosen = nullptr;

            for (const std::unique_ptr<ui::Node>& child : host.children)
            {
                const ui::DockState& dock = child->content.dock;
                bool own = child->visible || child->content.overflowed;
                if (child->kind == ui::ElementKind::DOCK_PANEL && own && dock.side == side && !dock.hidden && !dock.floating)
                    panels.push_back(child.get());
            }
            for (ui::Node* panel : panels)
            {
                if (chosen == nullptr && panel->content.dock.active)
                    chosen = panel;
            }
            if (chosen == nullptr && !panels.empty())
                chosen = panels.front();
            syncDockTitles(*strip, panels, chosen);
            return chosen;
        }

        // Отрезать от rest полосу длиной length у края side; rest уменьшается / Cutting off a band of length length at the side edge from rest; rest shrinks
        Rect cutSide(Rect& rest, ui::DockSide side, float length) noexcept
        {
            Rect band = rest;
            bool across = side == ui::DockSide::LEFT || side == ui::DockSide::RIGHT;

            length = std::max(0.0f, std::min(length, mainOf(Size{rest.width, rest.height}, across)));
            if (side == ui::DockSide::LEFT)
                rest.x += length;
            if (across)
                rest.width -= length;
            if (side == ui::DockSide::TOP)
                rest.y += length;
            if (side == ui::DockSide::TOP || side == ui::DockSide::BOTTOM)
                rest.height -= length;
            if (across)
                band.width = length;
            else
                band.height = length;
            if (side == ui::DockSide::RIGHT)
                band.x = rest.x + rest.width;
            if (side == ui::DockSide::BOTTOM)
                band.y = rest.y + rest.height;
            return band;
        }

        // Полоса заголовков сверху области края, под ней — выбранная панель / The title strip at the top of the edge area, the chosen panel under it
        void placeSideArea(ui::Node& strip, ui::Node& panel, Rect area, LayoutPass& pass)
        {
            float stripHeight = std::min(area.height, measureNode(strip, Size{area.width, area.height}, pass).height);

            strip.visible = true;
            arrangeNode(strip, Rect{area.x, area.y, area.width, stripHeight}, pass);
            arrangeNode(panel, Rect{area.x, area.y + stripHeight, area.width, area.height - stripHeight}, pass);
        }

        /*
        ** Один край: панели края — вкладки (видна выбранная), длина края — из хоста (её меняет ручка), иначе — size
        ** первой панели, но не больше места без 40 пикселей центра. Края без панелей не занимают места.
        **
        ** One edge: the edge panels are tabs (the chosen one is shown), the edge length is from the host (the handle changes it), otherwise
        ** the size of the first panel, but not more than the room minus 40 pixels of the center. Edges without panels take no room.
        */
        void placeSide(ui::Node& host, ui::DockSide side, Rect& rest, LayoutPass& pass)
        {
            std::vector<ui::Node*> panels;
            ui::Node* strip = ui::dockStrip(host, side);
            ui::Node* handle = ui::dockHandle(host, side);
            ui::Node* chosen = sidePanels(host, side, panels);
            std::size_t index = static_cast<std::size_t>(side);

            handle->visible = chosen != nullptr;
            strip->visible = chosen != nullptr;
            if (chosen == nullptr)
                return;
            bool across = side == ui::DockSide::LEFT || side == ui::DockSide::RIGHT;
            float room = mainOf(Size{rest.width, rest.height}, across);
            float length = host.content.dock.sizes[index];
            if (length < 0.0f)
                length = panels.front()->content.dock.size;
            length = std::max(0.0f, std::min(length, room - 40.0f));
            Rect area = cutSide(rest, side, length);
            Size grip = measureNode(*handle, Size{rest.width, rest.height}, pass);
            arrangeNode(*handle, cutSide(rest, side, mainOf(grip, across)), pass);
            ui::setLayoutHidden(*chosen, false);
            placeSideArea(*strip, *chosen, area, pass);
        }
    }

    /*
    ** DockHost: сначала все панели скрыты компоновкой, затем каждый край показывает свою выбранную; верхний и нижний
    ** края — во всю ширину, левый и правый — между ними; центр получает остаток.
    **
    ** A DockHost: first all the panels are hidden by the layout, then every edge shows its chosen one; the top and bottom
    ** edges take the full width, the left and right ones are between them; the center gets the rest.
    */
    void arrangeDock(ui::Node& host, Rect content, LayoutPass& pass)
    {
        static const ui::DockSide ORDER[] = {ui::DockSide::TOP, ui::DockSide::BOTTOM, ui::DockSide::LEFT, ui::DockSide::RIGHT};
        Rect rest = content;
        ui::Node* center = ui::dockCenter(host);

        for (const std::unique_ptr<ui::Node>& child : host.children)
        {
            if (child->kind == ui::ElementKind::DOCK_PANEL)
                ui::setLayoutHidden(*child, true);
        }
        if (ui::dockStrip(host, ui::DockSide::LEFT) == nullptr)
            return;
        for (ui::DockSide side : ORDER)
            placeSide(host, side, rest, pass);
        if (center != nullptr)
            arrangeNode(*center, rest, pass);
    }
}
