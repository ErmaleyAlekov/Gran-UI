#include "gran/markup/internal/components.hpp"

#include "beresta.hpp"

namespace gran::markup::internal
{
    // Перебор видов по их именам (видов немного, поиск только при загрузке) / Walking the kinds by their names (there are few kinds, the lookup happens only at load time)
    bool elementKind(std::string_view name, ui::ElementKind& kind) noexcept
    {
        for (int index = 0; index <= static_cast<int>(ui::LAST_MARKUP_KIND); ++index)
        {
            if (name == ui::elementName(static_cast<ui::ElementKind>(index)))
            {
                kind = static_cast<ui::ElementKind>(index);
                return true;
            }
        }
        return false;
    }

    // Пространство имён без префикса тоже в счёт: xmlns="urn:…" делает элементы экземплярами / A namespace without a prefix counts too: xmlns="urn:…" makes the elements instances
    bool isComponentInstance(const beresta::Element& element)
    {
        std::optional<std::string> namespaceUri = element.getNamespaceURI();

        return namespaceUri.has_value() && !namespaceUri->empty();
    }

    // <Slot> тела компонента, а не экземпляр компонента с именем Slot / A component body <Slot>, not an instance of a component named Slot
    bool isSlotElement(const beresta::Element& element)
    {
        return element.getLocalName() == "Slot" && !isComponentInstance(element);
    }

    // У элемента интерфейса нет пространства имён: <c:Button> — это экземпляр компонента Button, а не кнопка
    // A UI element has no namespace: <c:Button> is an instance of the Button component, not a button
    bool isUiElement(const beresta::Element& element)
    {
        ui::ElementKind kind = ui::ElementKind::BOX;

        if (isComponentInstance(element))
            return true;
        return elementKind(element.getLocalName(), kind);
    }

    /*
    ** <ItemTemplate> содержит ровно один элемент (проверила схема); он строится как обычный
    ** узел, но остаётся вне дерева — образцом, с которого привязка снимает копии на каждую запись.
    ** Состояния (Hover, Pressed...) и прочие дети, не являющиеся элементами, пропускаются.
    **
    ** <ItemTemplate> holds exactly one element (the schema has checked it); it is built as an ordinary
    ** node but stays outside the tree — a sample the binding copies for every record.
    ** States (Hover, Pressed...) and other children that are not elements are skipped.
    ** <Slot> тела компонента не даёт узла: на его место встают дети экземпляра.
    ** A component body <Slot> gives no node: the instance children take its place.
    */
    void buildChildren(const beresta::Element& element, ui::Node& node, BuildContext& context)
    {
        for (const beresta::Element* child = firstElementChild(element); child != nullptr;
             child = nextElementSibling(*child))
        {
            if (isSlotElement(*child))
                buildSlot(*child, node, context);
            else if (isUiElement(*child))
                ui::appendChild(node, buildNode(*child, context));
            else if (child->getLocalName() == "ItemTemplate")
                node.itemTemplate = buildNode(*firstElementChild(*child), context);
        }
    }
}
