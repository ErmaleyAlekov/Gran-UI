#include "gran/markup/internal/overlay.hpp"

#include <set>

namespace gran::markup
{
    namespace
    {
        // Стабильные id описывают область до и после перемещения элементов.
        // Stable IDs identify the region both before and after elements move.
        void collectScope(const ui::Node& node, const std::string& scope, bool inside, std::set<std::string>& ids)
        {
            inside = inside || node.id == scope;
            if (inside && !node.id.empty())
                ids.insert(node.id);
            for (const auto& child : node.children)
                collectScope(*child, scope, inside, ids);
        }
    }

    // Удаляем операции области из копии профиля, затем вызывающий повторно применяет её к базе.
    // Remove the region's operations from a profile copy, which the caller reapplies to the base.
    std::string resetOverlayScope(const UiDocument& base, const UiDocument& current, std::string_view xml, const std::string& scope)
    {
        auto doc = internal::parseOverlay(xml);
        auto& root = *doc.getRoot();
        std::set<std::string> ids;
        collectScope(*base.root, scope, false, ids);
        collectScope(*current.root, scope, false, ids);
        for (auto* op = internal::firstElementChild(root); op;)
        {
            const auto* next = internal::nextElementSibling(*op);
            if (ids.count(op->getAttribute("id").value_or("")) || ids.count(op->getAttribute("parent").value_or("")))
                root.detachChild(*op);
            op = next;
        }
        return beresta::XmlSerializer().toString(doc);
    }

    // Создаём XML через DOM, чтобы значения параметров оставались данными.
    // Construct XML through the DOM so parameter values remain data.
    std::string appendOverlayEdit(std::string_view xml, const std::string& operation, const TokenMap& attributes,
        const TokenMap& parameters)
    {
        auto doc = internal::parseOverlay(xml);
        auto op = std::make_unique<beresta::Element>(operation);
        for (const auto& [name, value] : attributes)
            op->setAttribute(name, value);
        for (const auto& [name, value] : parameters)
        {
            auto param = std::make_unique<beresta::Element>("Param");
            param->setAttribute("name", name);
            param->setAttribute("value", value);
            op->appendChild(std::move(param));
        }
        doc.getRoot()->appendChild(std::move(op));
        return beresta::XmlSerializer().toString(doc);
    }
}
