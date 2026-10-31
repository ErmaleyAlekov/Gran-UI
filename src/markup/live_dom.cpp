#include "gran/markup/internal/build_context.hpp"

#include "beresta.hpp"

#include <memory>
#include <utility>

namespace gran::markup::internal
{
    namespace
    {
        // Ссылки поддерева на элементы обнуляются (и у шаблона списка) / The subtree links to elements are cleared (the list template too)
        void unlinkTree(ui::Node& node) noexcept
        {
            node.domElement = nullptr;
            if (node.itemTemplate)
                unlinkTree(*node.itemTemplate);
            for (const std::unique_ptr<ui::Node>& child : node.children)
                unlinkTree(*child);
        }

        // Место нового ребёнка: перед <ItemTemplate> (схема требует шаблон последним), иначе в конце
        // The place of a new child: before <ItemTemplate> (the schema requires the template last), otherwise at the end
        std::size_t childPosition(const beresta::Element& parent) noexcept
        {
            std::size_t position = parent.getChildCount();

            for (const beresta::Element* child = firstElementChild(parent); child != nullptr; child = nextElementSibling(*child))
            {
                if (child->getLocalName() == "ItemTemplate")
                    position = child->getIndexInParent();
            }
            return position;
        }
    }

    // Единственное место, где константность ссылки снимается: сам DOM изменяем / The only place where the link constness is dropped: the DOM itself is mutable
    beresta::Element* editableElement(const ui::Node& node) noexcept
    {
        return const_cast<beresta::Element*>(node.domElement);
    }

    // Элемент переносится вместе с поддеревом: адреса элементов (и ссылки узлов) не меняются
    // The element moves together with its subtree: the element addresses (and the node links) do not change
    void keepFragment(beresta::Document& fragment, const beresta::Element& root, ui::Node& node, const ui::Node* parent)
    {
        beresta::Element* target = nullptr;

        if (parent != nullptr)
            target = editableElement(*parent);
        if (target == nullptr)
        {
            unlinkTree(node);
            return;
        }
        std::unique_ptr<beresta::Node> element = fragment.getRoot()->detachChild(root);
        target->insertChild(childPosition(*target), std::move(element));
    }
}

namespace gran::markup
{
    // Элемент уничтожается вместе с поддеревом — ссылки на него обнуляются заранее
    // The element is destroyed together with its subtree — the links to it are cleared beforehand
    void detachMarkup(ui::Node& node)
    {
        beresta::Element* element = internal::editableElement(node);

        if (element == nullptr || element->getParent() == nullptr)
            return;
        internal::unlinkTree(node);
        element->getParent()->detachChild(*element);
    }
}
