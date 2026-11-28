#include "gran/app/internal/layout_panel.hpp"
#include "gran/markup/internal/overlay.hpp"

#include "gran/markup/customization.hpp"

#include <set>

namespace gran::app::panel
{
    // Внутренности компонента и динамические узлы не адресуются в базовом XML.
    // Component internals and application-owned nodes are not addresses in the base XML.
    bool profileElement(const ui::Node& node, const markup::UiDocument& doc)
    {
        for (auto* current = &node; current; current = current->parent)
            if (current->appendedByApplication || current->layoutInspector) return false;
        return node.domElement && !node.id.empty() && node.domElement->getOwnerDocument() == doc.dom.get()
            && node.domElement->getAttribute("id") == node.id;
    }

    // Проверяем текущую область повторно при подтверждении открытого диалога.
    // Recheck the current destination when the user confirms an open dialog.
    ui::Node* creationParent(AppWindow& win)
    {
        if (win.shared.editSelection.empty()) return nullptr;
        auto* parent = ui::findNode(*win.document.root, win.shared.editSelection);
        if (!parent || !profileElement(*parent, win.document) || !receivesChildren(parent->kind)
            || parent->kind == ui::ElementKind::SCROLL || parent->kind == ui::ElementKind::DOCK_HOST
            || parent->domElement->hasAttribute("items")
            || !markup::permitsCustomization(*parent->domElement, markup::Customization::INSERT)) return nullptr;
        return parent;
    }

    // Пространство имён экземпляра не должно перекрывать живых потомков приложения.
    // An instance namespace must not overlap the application's live descendants.
    static void collectLiveIds(const ui::Node& node, std::set<std::string>& ids)
    {
        if (!node.id.empty()) ids.insert(node.id);
        for (const auto& child : node.children) collectLiveIds(*child, ids);
    }

    // Динамические узлы приложения также сохраняются при перестроении профиля.
    // Dynamic application nodes also survive a profile rebuild.
    std::string newElementId(AppWindow& win)
    {
        std::set<std::string> ids;
        markup::internal::collectMarkupIds(*win.document.dom->getRoot(), ids);
        collectLiveIds(*win.document.root, ids);
        auto* parent = ui::findNode(*win.document.root, win.shared.editSelection);
        if (parent && parent->kind == ui::ElementKind::TABS && !parent->children.empty())
            for (const auto& header : parent->children.front()->children) ids.insert(ui::optionValue(*header));
        for (std::size_t index = 1; ; ++index)
        {
            const auto id = "user-" + std::to_string(index);
            if (ids.count(id)) continue;
            const auto prefix = id + ".";
            const auto next = ids.lower_bound(prefix);
            if (next == ids.end() || next->compare(0, prefix.size(), prefix) != 0) return id;
        }
    }
}
