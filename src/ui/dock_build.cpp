#include "gran/ui/node.hpp"

namespace gran::ui
{
    namespace
    {
        // Узел-часть хоста (полоса, заголовок, ручка): скрыт, пока компоновка не покажет / A host part node (a strip, a title, a handle): hidden until the layout shows it
        std::unique_ptr<Node> makePart(const Node& host, ElementKind kind, DockSide side)
        {
            std::unique_ptr<Node> part = std::make_unique<Node>();

            part->kind = kind;
            part->visible = false;
            part->line = host.line;
            part->column = host.column;
            part->content.dock.side = side;
            return part;
        }

        /*
        ** Полоса края: заголовок-зеркало на каждую панель хоста (подпись — title панели). Панель может перейти
        ** на любой край, поэтому зеркало есть в каждой полосе, а компоновка показывает только нужные: во время
        ** работы узлы не создаются и не удаляются.
        **
        ** An edge strip: a mirror title for every host panel (the label is the panel title). A panel can move
        ** to any edge, so the mirror is in every strip, and the layout shows only the needed ones: no nodes are created
        ** or removed at run time.
        */
        std::unique_ptr<Node> makeStrip(const Node& host, DockSide side)
        {
            std::unique_ptr<Node> strip = makePart(host, ElementKind::DOCK_STRIP, side);

            for (std::size_t index = 0; index < host.children.size(); ++index)
            {
                const Node& panel = *host.children[index];
                if (panel.kind != ElementKind::DOCK_PANEL)
                    continue;
                std::unique_ptr<Node> title = makePart(host, ElementKind::DOCK_TITLE, side);
                title->content.text = panel.content.text;
                title->content.dock.panel = index;
                appendChild(*strip, std::move(title));
            }
            return strip;
        }
    }

    // После детей разметки: четыре полосы, затем четыре ручки (порядок краёв — DockSide) / After the markup children: four strips, then four handles (the edge order is DockSide)
    void buildDockParts(Node& host)
    {
        // Раскладка разметки запоминается для сброса / The markup layout is remembered for the reset
        for (std::size_t side = 0; side < DOCK_SIDES; ++side)
            host.content.dock.homeSizes[side] = host.content.dock.sizes[side];
        for (const std::unique_ptr<Node>& child : host.children)
        {
            child->content.dock.homeSide = child->content.dock.side;
            child->content.dock.homeActive = child->content.dock.active;
            child->content.dock.homeHidden = child->content.dock.hidden;
        }
        for (std::size_t side = 0; side < DOCK_SIDES; ++side)
            appendChild(host, makeStrip(host, static_cast<DockSide>(side)));
        for (std::size_t side = 0; side < DOCK_SIDES; ++side)
            appendChild(host, makePart(host, ElementKind::DOCK_HANDLE, static_cast<DockSide>(side)));
    }

    // Панели одного края — вкладки: выбрана одна / The panels of one edge are tabs: one is chosen
    void chooseDockPanel(Node& panel)
    {
        Node* host = panel.parent;

        if (host == nullptr)
            return;
        for (const std::unique_ptr<Node>& child : host->children)
        {
            if (child->kind == ElementKind::DOCK_PANEL && child->content.dock.side == panel.content.dock.side)
                child->content.dock.active = child.get() == &panel;
        }
        markDirty(*host, DIRTY_LAYOUT | DIRTY_PAINT);
    }

    // Сброс без перестройки узлов: меняются только поля раскладки, компоновка всё перерисует / The reset without rebuilding the nodes: only the layout fields change, the layout redraws everything
    void resetDockLayout(Node& host)
    {
        DockState& dock = host.content.dock;

        for (std::size_t side = 0; side < DOCK_SIDES; ++side)
            dock.sizes[side] = dock.homeSizes[side];
        for (const std::unique_ptr<Node>& child : host.children)
        {
            if (child->kind != ElementKind::DOCK_PANEL)
                continue;
            child->content.dock.side = child->content.dock.homeSide;
            child->content.dock.active = child->content.dock.homeActive;
            child->content.dock.hidden = child->content.dock.homeHidden;
        }
        markDirty(host, DIRTY_LAYOUT | DIRTY_PAINT);
    }
}
