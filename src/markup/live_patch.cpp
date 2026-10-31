#include "gran/markup/internal/build_context.hpp"

#include "beresta.hpp"

namespace gran::markup::internal
{
    namespace
    {
        // <Resources> окна или nullptr / The window <Resources> or nullptr
        const beresta::Element* resourcesOf(const beresta::Element& window) noexcept
        {
            const beresta::Element* child = firstElementChild(window);

            if (child != nullptr && child->getLocalName() == "Resources")
                return child;
            return nullptr;
        }

        /*
        ** Дети узла один к одному соответствуют детям-элементам интерфейса его элемента. Не так у
        ** Tabs (полоса заголовков), ComboBox (слой пунктов), Text с участками (анонимные участки) и списков
        ** из данных (копии записей): у этих детей нет своих элементов, и сверка по ссылкам это видит.
        **
        ** The node children match the UI element children of its element one to one. Not so for
        ** Tabs (the header strip), ComboBox (the item layer), a Text with runs (the anonymous runs) and lists
        ** from data (the record copies): these children have no elements of their own, and checking the links sees it.
        */
        bool childrenMatch(const std::vector<ui::Node*>& children, const std::vector<const beresta::Element*>& elements) noexcept
        {
            if (children.size() != elements.size())
                return false;
            for (std::size_t index = 0; index < elements.size(); ++index)
            {
                if (children[index]->domElement != elements[index])
                    return false;
            }
            return true;
        }

        /*
        ** Равное поддерево остаётся целиком; у узла с той же оболочкой и тем же числом детей
        ** сравниваются дети по порядку; иначе узел перестраивается из нового элемента (вставка
        ** или удаление ребёнка перестраивает родителя — состояние детей переносит приложение).
        **
        ** An equal subtree stays whole; for a node with the same shell and the same child count
        ** the children are compared in order; otherwise the node is rebuilt from the new element (inserting
        ** or removing a child rebuilds the parent — the application carries over the children state).
        ** Дети, добавленные во время работы, в сравнение не входят / The children added at run time do not take part.
        */
        void planNode(ui::Node& node, const beresta::Element& next, PatchPlan& plan)
        {
            const beresta::Element& previous = *node.domElement;
            std::vector<ui::Node*> children;

            if (previous.isEqualNode(next))
            {
                relinkEqual(node, previous, next, plan);
                return;
            }
            for (const std::unique_ptr<ui::Node>& child : node.children)
            {
                if (!isRuntimeChild(node, *child))
                    children.push_back(child.get());
            }
            std::vector<const beresta::Element*> before = uiElementChildren(previous);
            std::vector<const beresta::Element*> after = uiElementChildren(next);
            if (!sameShell(previous, next) || before.size() != after.size() || !childrenMatch(children, before))
            {
                plan.rebuild.emplace_back(&node, &next);
                return;
            }
            plan.relink.emplace_back(&node, &next);
            for (std::size_t index = 0; index < after.size(); ++index)
                planNode(*children[index], *after[index], plan);
        }
    }
}

namespace gran::markup
{
    // Виды, чьих детей без элемента строит загрузчик, и списки из данных / The kinds whose children without an element the loader builds, and lists from data
    bool isRuntimeChild(const ui::Node& parent, const ui::Node& child) noexcept
    {
        ui::ElementKind kind = parent.kind;

        if (child.domElement != nullptr || !parent.content.items.empty())
            return false;
        return kind != ui::ElementKind::TABS && kind != ui::ElementKind::COMBO_BOX && kind != ui::ElementKind::TEXT;
    }

    // Пролог (параметры окна и ресурсы) должен совпасть: от него зависят стили, на которые ссылаются узлы
    // The prologue (the window parameters and the resources) must match: the styles the nodes refer to depend on it
    bool planPatch(UiDocument& current, const UiDocument& next, PatchPlan& plan)
    {
        if (!current.dom || !next.dom || !current.root || !next.root || current.root->domElement == nullptr
            || next.root->domElement == nullptr)
            return false;
        const beresta::Element& before = *current.dom->getRoot();
        const beresta::Element& after = *next.dom->getRoot();
        const beresta::Element* oldResources = internal::resourcesOf(before);
        const beresta::Element* newResources = internal::resourcesOf(after);
        if (!internal::sameAttributes(before, after) || (oldResources == nullptr) != (newResources == nullptr))
            return false;
        if (oldResources != nullptr && !oldResources->isEqualNode(*newResources))
            return false;
        internal::planNode(*current.root, *next.root->domElement, plan);
        return true;
    }
}
