#include "gran/layout/internal/layout_internal.hpp"

namespace gran::layout::internal
{
    namespace
    {
        // Раскладывает ли узел детей стеком (Stack, Box, Popup, Dialog, вкладки, строка меню, панель и группа, кнопка с детьми)
        // Whether the node lays out its children as a stack (Stack, Box, Popup, Dialog, tabs, a menu bar, a toolbar and a group, a button with children)
        bool isStackLike(const ui::Node& node) noexcept
        {
            ui::ElementKind kind = node.kind;

            if (kind == ui::ElementKind::STACK || kind == ui::ElementKind::BOX || kind == ui::ElementKind::TABS
                || kind == ui::ElementKind::TAB || kind == ui::ElementKind::TAB_BAR || kind == ui::ElementKind::MENU_BAR
                || kind == ui::ElementKind::TOOL_BAR || kind == ui::ElementKind::TOOL_GROUP || ui::isLayer(node))
                return true;
            return node.kind == ui::ElementKind::BUTTON && !node.children.empty();
        }
    }

    // Алгоритм выбирается видом узла / The algorithm is chosen by the node kind
    Size measureContent(ui::Node& node, Size contentAvailable, LayoutPass& pass)
    {
        if (node.kind == ui::ElementKind::TOOL_BAR)
            ui::unfoldToolBar(node);
        if (isStackLike(node))
            return measureStack(node, contentAvailable, pass);
        if (node.kind == ui::ElementKind::GRID)
            return measureGrid(node, contentAvailable, pass);
        if (node.kind == ui::ElementKind::CANVAS)
            return measureCanvas(node, contentAvailable, pass);
        if (node.kind == ui::ElementKind::SCROLL)
            return measureScroll(node, contentAvailable, pass);
        if (node.kind == ui::ElementKind::TEXT_AREA)
            return measureTextArea(node, contentAvailable, pass);
        return measureLeaf(node, contentAvailable, pass);
    }

    // У листа детей нет: размер содержимого равен прямоугольнику содержимого
    // A leaf has no children: the content size equals the content rectangle
    void arrangeContent(ui::Node& node, Rect content, LayoutPass& pass)
    {
        node.layout.contentSize = Size{content.width, content.height};
        // Панель инструментов: сначала решить, кто уйдёт в меню «ещё», затем стек и видимость пунктов-зеркал
        // A toolbar: first decide who goes into the "more" menu, then the stack and the mirror item visibility
        if (node.kind == ui::ElementKind::TOOL_BAR)
            fitToolBar(node, content, pass);
        if (isStackLike(node))
            arrangeStack(node, content, pass);
        if (node.kind == ui::ElementKind::TOOL_BAR)
            ui::syncOverflowMenu(node);
        else if (node.kind == ui::ElementKind::GRID)
            arrangeGrid(node, content, pass);
        else if (node.kind == ui::ElementKind::CANVAS)
            arrangeCanvas(node, content, pass);
        else if (node.kind == ui::ElementKind::SCROLL)
            arrangeScroll(node, content, pass);
        else if (node.kind == ui::ElementKind::TEXT_AREA)
            arrangeTextArea(node, content, pass);
    }
}
