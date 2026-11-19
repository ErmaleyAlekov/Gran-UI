#include "gran/app/internal/app_state.hpp"

namespace gran::app
{
    namespace
    {
        // Двойной щелчок по строке плоского дерева раскрывает или сворачивает её ветку (как в проводниках) / A double click on a flat tree row expands or collapses its branch (as in file managers)
        void toggleTreeRow(const binding::ListBindings& lists, const ui::Node& hit)
        {
            binding::ItemRef item = lists.itemOf(hit);
            data::TreeList* rows = dynamic_cast<data::TreeList*>(item.model);

            if (rows != nullptr && item.index < rows->size() && rows->hasChildren(item.index))
                rows->toggle(item.index);
        }
    }

    // Набранный символ начинает правку текущей ячейки (значение заменяется им) / A typed character starts editing the current cell (it replaces the value)
    bool AppWindow::listText(ui::Node& container, const std::string& text)
    {
        return beginGridEdit(*this, container, &text);
    }

    // Ячейка с правкой открывает редактор; иначе строка дерева (и таблицы-дерева) раскрывается / A cell with editing opens the editor; otherwise a tree (and tree table) row expands
    void AppWindow::listDoubleClick(ui::Node& container, ui::Node& hit)
    {
        if (!gridDoubleClick(*this, container, hit))
            toggleTreeRow(shared.lists, hit);
    }

    bool AppWindow::editorKey(ui::Node& focus, const platform::Event& event)
    {
        return gridEditorKey(*this, focus, event);
    }
}
