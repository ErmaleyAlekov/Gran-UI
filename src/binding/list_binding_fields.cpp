#include "gran/binding/internal/list_binding.hpp"
#include "gran/data/template_text.hpp"

#include <utility>

namespace gran::binding
{
    namespace
    {
        using C = ui::NodeContent;

        // Текстовые поля, в которых разрешены подстановки (как в схеме; option — value пункта ComboBox, dragData — данные перетаскивания)
        // The text fields where placeholders are allowed (as in the schema; option is the ComboBox item value, dragData is the drag data)
        std::string C::* const BOUND_FIELDS[] = {&C::text, &C::placeholder, &C::source, &C::tooltip, &C::option,
            &C::accessibleName, &C::dragData};

        // Поля одного узла; текст Text с участками — склейка участков, его не трогаем
        // The fields of one node; the text of a Text with runs is the joined runs, it is left alone
        void collectNode(ui::Node& node, std::vector<FieldBinding>& fields)
        {
            for (std::string C::* member : BOUND_FIELDS)
            {
                if (member == &C::text && ui::hasSpans(node))
                    continue;
                if (data::hasPlaceholders(node.content.*member))
                    fields.push_back({&node, member, node.content.*member});
            }
            for (const std::unique_ptr<ui::Node>& child : node.children)
                collectNode(*child, fields);
        }
    }

    // Шаблоны вложенных списков (itemTemplate) не обходятся: у них свои записи
    // The nested list templates (itemTemplate) are not walked: they have their own records
    std::vector<FieldBinding> collectFields(ui::Node& root)
    {
        std::vector<FieldBinding> fields;

        collectNode(root, fields);
        return fields;
    }

    /*
    ** Неизменившийся текст не помечается: смена одного поля записи не перекомпоновывает
    ** узлы с другими полями. Участок Span обновляет склейку своего Text.
    **
    ** An unchanged text is not marked: a change of one record field does not relayout
    ** the nodes with other fields. A Span run updates the joined text of its Text.
    */
    void applyFields(const std::vector<FieldBinding>& fields, const data::Record& record)
    {
        for (const FieldBinding& field : fields)
        {
            std::string value = data::expandTemplate(field.pattern, record);
            std::string& slot = field.node->content.*field.member;
            if (slot == value)
                continue;
            slot = std::move(value);
            ui::markDirty(*field.node, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
            if (field.node->kind == ui::ElementKind::SPAN && field.node->parent != nullptr)
                ui::joinSpans(*field.node->parent);
        }
    }
}
