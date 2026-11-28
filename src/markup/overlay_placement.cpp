#include "gran/markup/internal/overlay.hpp"

namespace gran::markup::internal
{
    namespace
    {
        // Проверяем родителя и циклы до отделения поддерева от прежнего места.
        // Check the parent and cycles before detaching the subtree.
        void transfer(beresta::Element& source, beresta::Element& parent, const beresta::Element* before)
        {
            requirePermission(source, Customization::MOVE);
            if (source.getParent() != &parent)
                requirePermission(parent, Customization::INSERT);
            for (const beresta::Node* ancestor = &parent; ancestor; ancestor = ancestor->getParent())
                if (ancestor == &source)
                    denyOverlay(source, source.getName());
            if (before == &source)
                return;
            auto node = source.getParent()->detachChild(source);
            parent.insertChild(before ? before->getIndexInParent() : parent.getChildCount(), std::move(node));
        }
        // Настройки экземпляра проверяются после присоединения, с учётом прав области.
        // Check instance settings after attachment so inherited permissions apply.
        void configureTemplate(OverlayContext& context, const beresta::Element& op, beresta::Element& node)
        {
            if (op.hasAttribute("text") || op.hasAttribute("sequence"))
            {
                requirePermission(node, Customization::COMMAND);
                if (node.getName() != "Button" || !node.getNamespaceURI().value_or("").empty() || node.hasAttribute("windowRole"))
                    denyOverlay(op, "command");
                if (const auto caption = op.getAttribute("text"))
                {
                    if (caption->find_first_of("{}") != std::string::npos) denyOverlay(op, *caption);
                    node.setAttribute("text", *caption);
                }
                if (op.hasAttribute("sequence"))
                {
                    node.removeAttribute("popup");
                    node.removeAttribute("onClick");
                    node.setAttribute("command", addSequence(context, op, node.getAttribute("text").value_or(*findAttribute(op, "template"))));
                }
            }
            if (op.hasAttribute("token"))
            {
                beresta::Element token("Token");
                token.setAttribute("id", *findAttribute(op, "id"));
                token.setAttribute("name", *findAttribute(op, "token"));
                token.setAttribute("value", *findAttribute(op, "value"));
                applyToken(context, token);
            }
        }
    }

    // Якорь задаётся стабильным id соседнего элемента в принимающем контейнере.
    // The anchor is the stable ID of a sibling in the destination container.
    void applyPlacement(OverlayContext& context, const beresta::Element& op)
    {
        auto* parent = overlayTarget(context, op, "parent");
        auto* before = op.hasAttribute("before") ? overlayTarget(context, op, "before") : nullptr;
        if (!parent || (!before && op.hasAttribute("before")))
            return;
        if (before && before->getParent() != parent)
            denyOverlay(op, "before");
        if (op.getLocalName() == "Move")
        {
            if (auto* source = overlayTarget(context, op, "id"))
                transfer(*source, *parent, before);
            return;
        }
        requirePermission(*parent, Customization::INSERT);
        auto instance = paletteInstance(context, op);
        const auto& id = *findAttribute(op, "id");
        instance->setAttribute("id", id);
        indexPaletteTree(context, *instance);
        if (instance->getName() == "Tab" && instance->getNamespaceURI().value_or("").empty()) instance->setAttribute("value", id);
        auto* inserted = instance.get();
        parent->insertChild(before ? before->getIndexInParent() : parent->getChildCount(), std::move(instance));
        if (op.hasAttribute("template")) configureTemplate(context, op, *inserted);
    }
}
