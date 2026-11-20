#include "gran/binding/internal/list_binding.hpp"
#include "gran/data/tree_list.hpp"
#include "gran/ui/tree_view.hpp"

namespace gran::binding
{
    /*
    ** Признаки берутся из самого плоского дерева, а не из выражений строки: стрелка есть с первого кадра,
    ** а не после первого пересчёта привязок, и смысл поля свой (раньше — текст «true» в чужом поле option).
    ** Вне плоского дерева (обычный список под столбцом tree) — лист: стрелки нет. Плоское дерево сообщает
    ** changed(строка) при смене глубины, раскрытия и детей — оттуда же вызывается эта функция.
    **
    ** The flags come from the flat tree itself, not from the row expressions: the arrow is there from the first frame,
    ** not after the first binding recompute, and the field has its own meaning (formerly the "true" text in the alien option field).
    ** Outside a flat tree (an ordinary list under a tree column) it is a leaf: no arrow. The flat tree reports
    ** changed(row) when the depth, the expansion and the children change — this function is called from there too.
    */
    void ListBinding::markTree(const BoundItem& item, std::size_t record)
    {
        const data::TreeList* rows = dynamic_cast<const data::TreeList*>(&_model);
        ui::Node* expander = ui::rowExpander(*item.root);
        bool branch = false;
        bool expanded = false;

        if (expander == nullptr)
            return;
        if (rows != nullptr && record < rows->size())
        {
            branch = rows->hasChildren(record);
            expanded = branch && rows->expanded(record);
        }
        if (expander->content.branch != branch)
        {
            expander->content.branch = branch;
            ui::markDirty(*expander, ui::DIRTY_PAINT);
        }
        ui::setChecked(*expander, expanded);
    }

    // Номер записи с той же версией указывает на ту же запись: на этом держатся элементы строк для дикторов
    // A record index with the same version points to the same record: the row elements for screen readers rely on it
    std::uint64_t ListBinding::version() const noexcept
    {
        return _version;
    }
}
