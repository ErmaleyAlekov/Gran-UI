#include "gran/app/internal/app_state.hpp"
#include "gran/markup/customization.hpp"

namespace gran::app
{
    // Зона контейнера принимает элемент внутрь; у листа доступна граница между соседями.
    // A container accepts a child, while a leaf provides a boundary between siblings.
    bool receivesChildren(ui::ElementKind kind)
    {
        using Kind = ui::ElementKind;
        return kind == Kind::STACK || kind == Kind::GRID || kind == Kind::BOX || kind == Kind::CANVAS
            || kind == Kind::SCROLL || kind == Kind::DIALOG || kind == Kind::TAB || kind == Kind::TABS || kind == Kind::TOOL_BAR || kind == Kind::TOOL_GROUP
            || kind == Kind::DOCK_PANEL || kind == Kind::DOCK_HOST;
    }

    namespace
    {
        // Центр соседнего контейнера принимает внутрь, края сохраняют порядок рядом с ним.
        // Drop into a sibling container at its center or reorder beside it at its edges.
        bool insideContainer(ui::Node& target, Vec2 point)
        {
            point = ui::windowToNode(target, point);
            const auto rect = target.layout.frame;
            return point.x >= rect.x + rect.width * 0.2f && point.x <= rect.x + rect.width * 0.8f
                && point.y >= rect.y + rect.height * 0.2f && point.y <= rect.y + rect.height * 0.8f;
        }

        // Половины видимой рамки задают вставку до или после элемента по оси компоновки.
        // The two halves of the visible frame select insertion before or after the item along the layout axis.
        std::string insertionAnchor(ui::Node& target, Vec2 point, render::DropEdge* edge)
        {
            auto* visual = target.kind == ui::ElementKind::TAB ? ui::headerOfPage(target) : &target;
            if (!visual) return target.id;
            const Rect rect = visual->layout.frame;
            point = ui::windowToNode(*visual, point);
            const bool horizontal = visual->parent->computed.orientation == static_cast<int>(style::Orientation::HORIZONTAL);
            const bool before = horizontal ? point.x < rect.x + rect.width / 2 : point.y < rect.y + rect.height / 2;
            if (edge) *edge = horizontal ? (before ? render::DropEdge::LEFT : render::DropEdge::RIGHT)
                                         : (before ? render::DropEdge::TOP : render::DropEdge::BOTTOM);
            if (before) return target.id;
            std::string next;
            for (auto it = target.parent->children.rbegin(); it != target.parent->children.rend() && it->get() != &target; ++it)
                if ((*it)->domElement && !(*it)->id.empty()) next = (*it)->id;
            return next;
        }
    }

    // Предпросмотр и отпускание мыши используют одинаковую проверку места вставки.
    // Preview and mouse release use the same drop-position check.
    markup::TokenMap layoutDrop(AppWindow& win, ui::Node* target, Vec2 point, render::DropEdge* edge)
    {
        auto* source = ui::findNode(*win.document.root, win.shared.editSelection);
        if (!source || !target || !target->domElement || source == target) return {};
        const bool page = source->kind == ui::ElementKind::TAB;
        const bool compatible = page == (target->kind == ui::ElementKind::TABS);
        const bool into = compatible && receivesChildren(target->kind)
            && (source->parent != target->parent || insideContainer(*target, point));
        if (into && target != source->parent && !markup::permitsCustomization(*target->domElement, markup::Customization::INSERT)) return {};
        auto* parent = into ? target : target->parent;
        if (!parent || !parent->domElement || parent->id.empty()
            || page != (parent->kind == ui::ElementKind::TABS)) return {};
        if (source->parent != parent && !markup::permitsCustomization(*parent->domElement, markup::Customization::INSERT))
            return {};
        for (const auto* ancestor = parent; ancestor; ancestor = ancestor->parent)
            if (ancestor == source) return {};
        markup::TokenMap result{{"id", source->id}, {"parent", parent->id}};
        if (edge) *edge = render::DropEdge::INSIDE;
        const auto before = into ? "" : insertionAnchor(*target, point, edge);
        if (!before.empty()) result["before"] = before;
        return result;
    }

    // У вкладки видимую ручку размещаем на заголовке страницы.
    // For a tab, place the visible handle on the page header.
    ui::Node* layoutMark(AppWindow& win, const std::string& id)
    {
        if (!win.shared.editMode || id.empty() || win.shared.windows.front().get() != &win) return nullptr;
        auto* node = ui::findNode(*win.document.root, id);
        return node && node->kind == ui::ElementKind::TAB ? ui::headerOfPage(*node) : node;
    }
}
