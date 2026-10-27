#include "gran/markup/internal/build_context.hpp"

#include "beresta.hpp"

namespace gran::markup::internal
{
    // Перебор видов по их именам (видов немного, поиск только при загрузке) / Walking the kinds by their names (there are few kinds, the lookup happens only at load time)
    bool elementKind(std::string_view name, ui::ElementKind& kind) noexcept
    {
        for (int index = 0; index <= static_cast<int>(ui::ElementKind::COMBO_BOX); ++index)
        {
            if (name == ui::elementName(static_cast<ui::ElementKind>(index)))
            {
                kind = static_cast<ui::ElementKind>(index);
                return true;
            }
        }
        return false;
    }

    /*
    ** <ItemTemplate> содержит ровно один элемент (проверила схема); он строится как обычный
    ** узел, но остаётся вне дерева — образцом, с которого привязка снимает копии на каждую запись.
    ** Состояния (Hover, Pressed...) и прочие дети, не являющиеся элементами, пропускаются.
    **
    ** <ItemTemplate> holds exactly one element (the schema has checked it); it is built as an ordinary
    ** node but stays outside the tree — a sample the binding copies for every record.
    ** States (Hover, Pressed...) and other children that are not elements are skipped.
    */
    void buildChildren(const beresta::Element& element, ui::Node& node, BuildContext& context)
    {
        ui::ElementKind childKind = ui::ElementKind::BOX;

        for (const beresta::Element* child = firstElementChild(element); child != nullptr;
             child = nextElementSibling(*child))
        {
            if (elementKind(child->getLocalName(), childKind))
                ui::appendChild(node, buildNode(*child, context));
            else if (child->getLocalName() == "ItemTemplate")
                node.itemTemplate = buildNode(*firstElementChild(*child), context);
        }
    }
}
