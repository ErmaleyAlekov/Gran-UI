#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <utility>

namespace gran
{
    namespace
    {
        // Виды, у которых дети — элементы интерфейса (у Scroll ровно один ребёнок, у Text — участки)
        // The kinds whose children are UI elements (a Scroll has exactly one child, a Text has runs)
        bool acceptsElements(const ui::Node& node) noexcept
        {
            switch (node.kind)
            {
            case ui::ElementKind::STACK: case ui::ElementKind::BOX: case ui::ElementKind::GRID:
            case ui::ElementKind::CANVAS: case ui::ElementKind::BUTTON: case ui::ElementKind::POPUP:
            case ui::ElementKind::DIALOG:
                return true;
            default:
                return false;
            }
        }
    }

    data::ListModel& Application::model(const std::string& name)
    {
        return _state->lists.model(name);
    }

    // Фрагмент строится полностью до вставки: ошибка разметки не оставляет в дереве половину узлов
    // The fragment is built completely before insertion: a markup error leaves no half of the nodes in the tree
    ui::Node& Application::appendMarkup(ui::Node& parent, std::string_view markup)
    {
        if (!acceptsElements(parent))
            throw GranException(ErrorCode::INVALID_ARGUMENT,
                i18n::format(i18n::Message::MARKUP_FRAGMENT_PARENT, {ui::elementName(parent.kind)}));
        std::unique_ptr<ui::Node> node = markup::buildFragment(markup, app::windowOf(*_state, parent).document, &parent);
        ui::applyInitialStates(*node);
        return append(parent, std::move(node));
    }

    // Ближайший список — по первому предку, чей родитель привязан / The nearest list — by the first ancestor whose parent is bound
    ItemRef Application::itemOf(const ui::Node& node) const
    {
        return _state->lists.itemOf(node);
    }

    void Application::refreshBindings()
    {
        _state->lists.updateExpressions();
    }
}
