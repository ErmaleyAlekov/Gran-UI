#include "gran/app/internal/app_state.hpp"
#include "gran/markup/customization.hpp"
#include "gran/log/log.hpp"

namespace gran::app
{
    namespace
    {
        // От внутренней части контрола поднимаемся до адресуемого элемента разметки.
        // Walk from an internal control part up to an addressable markup element.
        ui::Node* authoredElement(ui::Node* node)
        {
            while (node && (!node->domElement || node->id.empty()))
            {
                if (node->kind == ui::ElementKind::TAB_HEADER && ui::pageOf(*node))
                    node = ui::pageOf(*node);
                else
                    node = node->parent;
            }
            return node;
        }

        // Явные права определяют, доступен ли инспектор.
        // Effective rights determine whether the inspector is available.
        ui::Node* editableElement(ui::Node* node)
        {
            using Right = markup::Customization;
            if (node)
                for (auto right : {Right::MOVE, Right::INSERT, Right::HIDE, Right::STYLE, Right::COMMAND})
                    if (markup::permitsCustomization(*node->domElement, right)) return node;
            return nullptr;
        }

        // Выбор хранится как id, чтобы пережить замену документа без висячих указателей.
        // Store the selection as an ID so it survives document replacement without dangling pointers.
        bool pointerEdit(AppWindow& win, const platform::Event& event)
        {
            auto& state = win.shared;
            if (event.button == platform::MouseButton::LEFT && resizeLayout(win, event))
                return true;
            auto* target = authoredElement(input::hitTest(*win.document.root, event.position));
            if (event.kind == platform::EventKind::MOUSE_DOWN)
            {
                target = editableElement(target);
                state.editSelection = target ? target->id : "";
                if (event.button == platform::MouseButton::RIGHT) openLayoutInspector(win, target);
                else state.editDragging = target && markup::permitsCustomization(*target->domElement, markup::Customization::MOVE);
            }
            else if (event.kind == platform::EventKind::MOUSE_MOVE && state.editDragging)
                state.editDrop = layoutDrop(win, target, event.position, &state.dropEdge).empty() ? "" : target->id;
            else if (event.kind == platform::EventKind::MOUSE_MOVE)
            {
                state.dropEdge = render::DropEdge::INSIDE;
                state.editDrop = editableElement(target) && target->id != state.editSelection ? target->id : "";
            }
            else if (event.kind == platform::EventKind::MOUSE_UP && state.editDragging)
            {
                state.editDragging = false;
                state.editDrop.clear();
                const auto place = layoutDrop(win, target, event.position);
                if (!place.empty()) state.owner->editLayout("Move", place);
            }
            return true;
        }

        // Фокус клавиатуры даёт доступ к тем же свойствам, что правая кнопка мыши.
        // Keyboard focus exposes the same properties as a right-click.
        bool keyEdit(AppWindow& win, const platform::Event& event)
        {
            using Key = platform::Key;
            auto& state = win.shared;
            if (event.key == Key::ESCAPE) state.owner->setEditMode(false);
            else if (event.key == Key::CONTEXT_MENU || (event.key == Key::F10 && event.modifiers == platform::MODIFIER_SHIFT))
            {
                auto* target = editableElement(authoredElement(const_cast<ui::Node*>(win.router->focused())));
                if (!target && !state.editSelection.empty()) target = ui::findNode(*win.document.root, state.editSelection);
                openLayoutInspector(win, target);
            }
            else if (event.key == Key::UP && event.modifiers == platform::MODIFIER_ALT)
            {
                auto* selected = state.editSelection.empty() ? const_cast<ui::Node*>(win.router->focused())
                    : ui::findNode(*win.document.root, state.editSelection);
                auto* parent = selected ? authoredElement(selected->parent) : nullptr;
                if (parent) state.editSelection = parent->id;
            }
            else if (event.key == Key::DELETE_KEY && !state.editSelection.empty())
                state.owner->editLayout("Hide", {{"id", state.editSelection}});
            else return false;
            return true;
        }
    }

    // В режиме правки выбор мышью обрабатывается до обычных действий кнопки.
    // In edit mode, process pointer selection before ordinary button actions.
    bool handleLayoutInput(AppWindow& win, const platform::Event& event)
    {
        using Kind = platform::EventKind;
        auto& state = win.shared;
        if (!state.editMode || state.windows.front().get() != &win)
            return false;
        auto* dialog = layoutInspector(win);
        if (dialog && dialog->visible) return false;
        try
        {
            if (event.kind == Kind::FOCUS_LOST) { state.editDragging = false; state.editGrip = {}; state.editDrop.clear(); }
            if (event.kind == Kind::MOUSE_MOVE) return pointerEdit(win, event);
            if (event.kind == Kind::MOUSE_UP || event.kind == Kind::MOUSE_DOWN)
            {
                if (event.button != platform::MouseButton::LEFT && event.button != platform::MouseButton::RIGHT) return true;
                return pointerEdit(win, event);
            }
            return event.kind == Kind::KEY_DOWN && keyEdit(win, event);
        }
        catch (const std::exception& error)
        {
            log::write(log::Level::WARNING, "markup", error.what());
        }
        return true;
    }
}
