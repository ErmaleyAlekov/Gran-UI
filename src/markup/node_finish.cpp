#include "gran/markup/internal/build_context.hpp"

#include "beresta.hpp"

namespace gran::markup::internal
{
    namespace
    {
        // Пункт меню или строки меню (не ComboBox): только у них «_» в подписи — буква доступа
        // An item of a menu or a menu bar (not a ComboBox): only for them "_" in the label is the access letter
        bool inMenu(const beresta::Element& element) noexcept
        {
            const beresta::ContainerNode* parent = element.getParent();

            if (parent == nullptr || parent->getType() != beresta::NodeType::ELEMENT)
                return false;
            std::string_view name = static_cast<const beresta::Element*>(parent)->getLocalName();
            return name == "Popup" || name == "MenuBar";
        }
    }

    // Узлы, которые движок достраивает сам, получают свои части здесь, когда дети уже построены / The nodes the engine finishes itself get their parts here, when the children are already built
    void finishNode(const beresta::Element& element, ui::Node& node)
    {
        if (node.kind == ui::ElementKind::MENU_ITEM && inMenu(element))
            ui::takeMnemonic(node);
        if (node.kind == ui::ElementKind::TABS)
            ui::syncTabs(node);
        if (node.kind == ui::ElementKind::TOOL_BAR)
            ui::buildToolBarMore(node);
        if (node.kind == ui::ElementKind::STATUS_BAR && node.content.sizeGrip)
            ui::buildSizeGrip(node);
        if (node.kind == ui::ElementKind::SPLITTER)
            ui::buildSplitterHandle(node);
        if (node.kind == ui::ElementKind::DOCK_HOST)
            ui::buildDockParts(node);
    }
}
