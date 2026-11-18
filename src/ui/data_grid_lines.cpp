#include "gran/ui/data_grid.hpp"

namespace gran::ui
{
    namespace
    {
        // Копии строк списка (без распорок виртуального списка) — в конец lines / The list row copies (without the virtual list spacers) go to the end of lines
        void appendRows(Node& list, std::vector<Node*>& lines)
        {
            for (const std::unique_ptr<Node>& row : list.children)
            {
                if (row->kind == ElementKind::GRID_ROW)
                    lines.push_back(row.get());
            }
        }
    }

    /*
    ** Дети таблицы (buildDataGrid): заголовок, закреплённый список сверху, прокрутка со списком строк,
    ** закреплённый список снизу. Порядок строк в ответе — порядок на экране сверху вниз.
    **
    ** The table children (buildDataGrid): the header, the pinned list at the top, the scroll with the row list,
    ** the pinned list at the bottom. The order of the lines in the answer is the on-screen order from the top down.
    */
    std::vector<Node*> gridLines(Node& grid)
    {
        std::vector<Node*> lines;

        for (const std::unique_ptr<Node>& child : grid.children)
        {
            if (child->kind == ElementKind::GRID_HEADER)
                lines.push_back(child.get());
            else if (child->kind == ElementKind::STACK)
                appendRows(*child, lines);
            else if (child->kind == ElementKind::SCROLL && !child->children.empty())
                appendRows(*child->children.front(), lines);
        }
        return lines;
    }

    // Копия списка строк: строка → список → прокрутка; заголовок и закреплённые строки лежат вне прокрутки / A row list copy: the row → the list → the scroll; the header and the pinned rows lie outside the scroll
    bool scrolledLine(const Node& line) noexcept
    {
        return line.kind == ElementKind::GRID_ROW && line.parent != nullptr && line.parent->parent != nullptr
            && line.parent->parent->kind == ElementKind::SCROLL;
    }

    // Дети таблицы по порядку: кто встретился раньше — список или прокрутка / The table children in order: who comes first — the list or the scroll
    bool pinnedAbove(const Node& list) noexcept
    {
        if (list.parent == nullptr)
            return false;
        for (const std::unique_ptr<Node>& child : list.parent->children)
        {
            if (child.get() == &list)
                return true;
            if (child->kind == ElementKind::SCROLL)
                return false;
        }
        return false;
    }
}
