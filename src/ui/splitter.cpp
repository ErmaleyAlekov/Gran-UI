#include "gran/ui/node.hpp"

namespace gran::ui
{
    /*
    ** Ручка строится движком (как кнопка «ещё» панели): встаёт средним ребёнком между двумя панелями.
    ** Разметка не из двух панелей (схема это запрещает) — ручки нет, разделитель раскладывается как пустой.
    **
    ** The handle is built by the engine (as the toolbar "more" button): it becomes the middle child between the two panes.
    ** Markup without two panes (the schema forbids it) — no handle, the splitter lays out as an empty one.
    */
    void buildSplitterHandle(Node& splitter)
    {
        std::unique_ptr<Node> handle = std::make_unique<Node>();

        if (splitter.children.size() != 2)
            return;
        handle->kind = ElementKind::SPLITTER_HANDLE;
        handle->line = splitter.line;
        handle->column = splitter.column;
        insertChild(splitter, 1, std::move(handle));
    }

    // Порядок детей после построителя: панель, ручка, панель / The children order after the builder: a pane, the handle, a pane
    Node* splitterPane(Node& splitter, std::size_t index) noexcept
    {
        if (splitter.kind != ElementKind::SPLITTER || splitter.children.size() != 3 || index > 1)
            return nullptr;
        return splitter.children[index * 2].get();
    }

    // Ручка — средний ребёнок вида SPLITTER_HANDLE / The handle is the middle child of the SPLITTER_HANDLE kind
    Node* splitterHandle(Node& splitter) noexcept
    {
        if (splitter.kind != ElementKind::SPLITTER || splitter.children.size() != 3
            || splitter.children[1]->kind != ElementKind::SPLITTER_HANDLE)
            return nullptr;
        return splitter.children[1].get();
    }

    // Ось — свойство orientation разделителя (тема: horizontal) / The axis is the splitter orientation property (the theme: horizontal)
    bool splitterHorizontal(const Node& handle) noexcept
    {
        return handle.parent != nullptr
            && handle.parent->computed.orientation == static_cast<int>(style::Orientation::HORIZONTAL);
    }
}
