#include "gran/app/internal/app_state.hpp"

namespace gran::app
{
    namespace
    {
        // Детей режет прокрутка или overflow: hidden / The children are cut by a scroll or overflow: hidden
        bool clipsChildren(const ui::Node& node) noexcept
        {
            return node.kind == ui::ElementKind::SCROLL
                || node.computed.overflow == static_cast<int>(style::Overflow::HIDDEN);
        }

        /*
        ** Обход видимых узлов с областью отсечения предков: зона — рамка узла onDrop в координатах окна,
        ** обрезанная видимой частью; полностью скрытая зона не попадает в список.
        **
        ** A walk over the visible nodes with the ancestor clipping area: a zone is the onDrop node frame in window coordinates,
        ** cut by the visible part; a fully hidden zone does not get into the list.
        */
        void collectZones(const ui::Node& node, const Rect& clip, std::vector<Rect>& zones)
        {
            Rect bounds;

            if (!node.visible)
                return;
            if (!node.content.onDrop.empty() || clipsChildren(node))
                bounds = rectIntersect(clip, affineBounds(ui::nodeToWindow(node), node.layout.frame));
            if (!node.content.onDrop.empty() && node.enabled && bounds.width > 0.0f && bounds.height > 0.0f)
                zones.push_back(bounds);
            for (const std::unique_ptr<ui::Node>& child : node.children)
            {
                if (clipsChildren(node))
                    collectZones(*child, bounds, zones);
                else
                    collectZones(*child, clip, zones);
            }
        }

        // Совпадают ли списки зон / Whether the zone lists match
        bool sameZones(const std::vector<Rect>& first, const std::vector<Rect>& second) noexcept
        {
            if (first.size() != second.size())
                return false;
            for (std::size_t index = 0; index < first.size(); ++index)
            {
                if (first[index].x != second[index].x || first[index].y != second[index].y
                    || first[index].width != second[index].width || first[index].height != second[index].height)
                    return false;
            }
            return true;
        }
    }

    // Окну ОС — только изменения: оно отвечает ОС по этому списку во время перетаскивания / Only changes go to the OS window: it answers the OS by this list while dragging
    void syncDropZones(AppWindow& win)
    {
        Size pixels = win.window->pixelSize();
        float scale = win.window->scale();
        std::vector<Rect> zones;

        collectZones(*win.document.root, Rect{0.0f, 0.0f, pixels.width / scale, pixels.height / scale}, zones);
        if (sameZones(zones, win.dropZones))
            return;
        win.dropZones = zones;
        win.window->setDropZones(zones);
    }
}
