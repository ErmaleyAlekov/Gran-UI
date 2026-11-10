#include "gran/ui/node.hpp"

namespace gran::ui
{
    namespace
    {
        // Часть хоста нужного вида и края / A host part of the needed kind and edge
        Node* dockPart(Node& host, ElementKind kind, DockSide side) noexcept
        {
            for (const std::unique_ptr<Node>& child : host.children)
            {
                if (child->kind == kind && child->content.dock.side == side)
                    return child.get();
            }
            return nullptr;
        }
    }

    // Полоса заголовков края / The edge title strip
    Node* dockStrip(Node& host, DockSide side) noexcept
    {
        return dockPart(host, ElementKind::DOCK_STRIP, side);
    }

    // Ручка края / The edge handle
    Node* dockHandle(Node& host, DockSide side) noexcept
    {
        return dockPart(host, ElementKind::DOCK_HANDLE, side);
    }

    // Заголовок — ребёнок полосы, полоса — ребёнок хоста; номер панели записан в заголовке / A title is a strip child, the strip is a host child; the panel number is stored in the title
    Node* dockPanelOf(Node& title) noexcept
    {
        Node* strip = title.parent;

        if (title.kind != ElementKind::DOCK_TITLE || strip == nullptr || strip->parent == nullptr)
            return nullptr;
        Node& host = *strip->parent;
        std::size_t index = title.content.dock.panel;
        if (index >= host.children.size() || host.children[index]->kind != ElementKind::DOCK_PANEL)
            return nullptr;
        return host.children[index].get();
    }

    // Центр — то, что остаётся между краями (документ, вкладки документов) / The center is what remains between the edges (a document, document tabs)
    Node* dockCenter(Node& host) noexcept
    {
        for (const std::unique_ptr<Node>& child : host.children)
        {
            ElementKind kind = child->kind;
            if (kind != ElementKind::DOCK_PANEL && kind != ElementKind::DOCK_STRIP && kind != ElementKind::DOCK_HANDLE)
                return child.get();
        }
        return nullptr;
    }
}
