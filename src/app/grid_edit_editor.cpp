#include "gran/app/internal/app_state.hpp"
#include "gran/binding/internal/list_binding.hpp"

#include <utility>

namespace gran::app
{
    namespace
    {
        /*
        ** Поле редактора по умолчанию — на всю ширину ячейки при нулевой основе: своя ширина поля не входит в ширину
        ** столбца auto (иначе столбец расширялся бы на время правки и уже не сужался — замечено на снимке). Выравнивание —
        ** как у текста ячейки (числа справа). Имя для дикторов — заголовок столбца.
        **
        ** The default editor field is over the whole cell width with a zero basis: the own field width does not go into an auto
        ** column width (otherwise the column would widen for the edit and never narrow back — seen on a snapshot). The alignment
        ** is as the cell text one (numbers to the right). The screen reader name is the column title.
        */
        std::unique_ptr<ui::Node> makeField(const ui::Node& cell, ui::ElementKind kind, const ui::GridColumn& column)
        {
            std::unique_ptr<ui::Node> node = std::make_unique<ui::Node>();

            node->kind = kind;
            node->line = cell.line;
            node->column = cell.column;
            node->content.accessibleName = column.title;
            if (kind == ui::ElementKind::CHECKBOX)
                return node;
            style::setDeclaration(node->declared.base, style::PropertyId::GROW, 1.0f);
            style::setDeclaration(node->declared.base, style::PropertyId::WIDTH, style::Length{0.0f, style::LengthUnit::PIXELS});
            const style::PropertyValue* align = nullptr;
            if (!cell.children.empty())
                align = style::findDeclaration(cell.children.front()->declared.base, style::PropertyId::TEXT_ALIGN);
            if (align != nullptr)
                style::setDeclaration(node->declared.base, style::PropertyId::TEXT_ALIGN, *align);
            return node;
        }

        // Вид, хранящий значение, которое правит пользователь / A kind holding the value the user edits
        bool isValueKind(ui::ElementKind kind) noexcept
        {
            return kind == ui::ElementKind::INPUT || kind == ui::ElementKind::TEXT_AREA || kind == ui::ElementKind::COMBO_BOX
                || kind == ui::ElementKind::CHECKBOX || kind == ui::ElementKind::SWITCH || kind == ui::ElementKind::SLIDER;
        }
    }

    /*
    ** <EditTemplate> копируется, как шаблон строки: подстановки {поле} получают поля записи (подсказка, пределы
    ** ползунка и т. п.); корень без своих grow и width растягивается на ячейку с нулевой основой. Без шаблона — поле ввода, для BOOLEAN — флажок.
    **
    ** <EditTemplate> is copied as the row template: the {field} placeholders get the record fields (a placeholder, the slider
    ** limits and the like); a root without its own grow and width is stretched over the cell with a zero basis. Without a template — an input, for BOOLEAN — a check box.
    */
    std::unique_ptr<ui::Node> makeGridEditor(const ui::Node& cell, const ui::GridColumn& column, data::ColumnType type,
        const data::Record& record)
    {
        if (column.editTemplate)
        {
            std::unique_ptr<ui::Node> copy = ui::cloneTree(*column.editTemplate);
            binding::applyFields(binding::collectFields(*copy), record);
            if (style::findDeclaration(copy->declared.base, style::PropertyId::GROW) == nullptr)
                style::setDeclaration(copy->declared.base, style::PropertyId::GROW, 1.0f);
            if (style::findDeclaration(copy->declared.base, style::PropertyId::WIDTH) == nullptr)
                style::setDeclaration(copy->declared.base, style::PropertyId::WIDTH, style::Length{0.0f, style::LengthUnit::PIXELS});
            return copy;
        }
        if (type == data::ColumnType::BOOLEAN)
            return makeField(cell, ui::ElementKind::CHECKBOX, column);
        return makeField(cell, ui::ElementKind::INPUT, column);
    }

    // Обход в глубину по порядку детей: первый по разметке элемент значения / A depth-first walk in the children order: the first value element in the markup
    ui::Node* editorValueNode(ui::Node& editor) noexcept
    {
        if (isValueKind(editor.kind))
            return &editor;
        for (const std::unique_ptr<ui::Node>& child : editor.children)
        {
            ui::Node* found = editorValueNode(*child);
            if (found != nullptr)
                return found;
        }
        return nullptr;
    }

    std::unique_ptr<GridEdit> makeGridSession(const GridEditPlace& place, const ui::GridColumn& column, data::ColumnType type)
    {
        std::unique_ptr<GridEdit> edit = std::make_unique<GridEdit>();
        const data::Record& record = place.source->at(place.index);
        auto found = record.find(column.field);

        if (found != record.end())
            edit->original = found->second;
        edit->grid = place.grid;
        edit->rows = place.rows;
        edit->field = column.field;
        edit->key = column.key;
        edit->type = type;
        edit->undoStack = place.grid->content.grid->undoStack;
        edit->undoable = place.grid->content.undoable;
        edit->record = std::make_shared<data::RecordTracker>(*place.source, place.index);
        return edit;
    }
}
