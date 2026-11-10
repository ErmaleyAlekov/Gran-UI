#include "gran/input/input_router.hpp"

namespace gran::input
{
    /*
    ** Нажатие на ручку: двойной щелчок сворачивает или разворачивает панель, одиночный начинает перетаскивание —
    ** запоминается место нажатия внутри ручки, чтобы она не прыгала под указатель.
    **
    ** A press on the handle: a double click collapses or expands the pane, a single one starts dragging —
    ** the press place inside the handle is remembered so the handle does not jump under the pointer.
    */
    void pressSplitter(RouterState& state, ui::Node& handle, const platform::Event& event)
    {
        Vec2 local = ui::windowToNode(handle, event.position);
        bool horizontal = handle.parent != nullptr
            && handle.parent->computed.orientation == static_cast<int>(style::Orientation::HORIZONTAL);

        if (event.clicks == 2 && handle.parent != nullptr)
        {
            toggleSplit(state, *handle.parent);
            return;
        }
        state.drag = &handle;
        state.splitGrab = local.y - handle.layout.frame.y;
        if (horizontal)
            state.splitGrab = local.x - handle.layout.frame.x;
    }

    // Ползунок, ручка разделителя и заголовок вкладки тянутся мышью с захватом (state.drag) / A slider, a splitter handle and a tab header are dragged by the mouse with a capture (state.drag)
    void pressDragControl(RouterState& state, ui::Node& control, const platform::Event& event)
    {
        if (control.kind == ui::ElementKind::SPLITTER_HANDLE)
            pressSplitter(state, control, event);
        if (control.kind == ui::ElementKind::TAB_HEADER)
            pressTab(state, control, event);
        if (control.kind != ui::ElementKind::SLIDER)
            return;
        state.drag = &control;
        dragSlider(state, control, ui::windowToNode(control, event.position).x);
    }

    // Конец перетаскивания или шаг клавишей: onChange разделителя (значение — handle.content.value) / The drag end or a key step: the splitter onChange (the value is handle.content.value)
    void finishSplit(RouterState& state, ui::Node& splitter)
    {
        if (!splitter.content.onChange.empty())
            state.actions->action(splitter.content.onChange, splitter);
    }

    // Движение мыши с нажатой кнопкой: ползунок — значение, ручка — положение разделителя / A mouse move with the button held: a slider — the value, a handle — the splitter position
    void moveDragControl(RouterState& state, ui::Node& control, Vec2 point)
    {
        if (control.kind == ui::ElementKind::SPLITTER_HANDLE)
            dragSplitter(state, control, point);
        else if (control.kind == ui::ElementKind::TAB_HEADER)
            dragTab(state, control, point);
        else
            dragSlider(state, control, ui::windowToNode(control, point).x);
    }

    // Отпускание: у ручки — onChange разделителя (один раз за перетаскивание), захват снимается / The release: for a handle — the splitter onChange (once per drag), the capture is removed
    void endDragControl(RouterState& state)
    {
        ui::Node* control = state.drag;

        state.drag = nullptr;
        if (control != nullptr && control->kind == ui::ElementKind::SPLITTER_HANDLE && control->parent != nullptr)
            finishSplit(state, *control->parent);
        if (control != nullptr && control->kind == ui::ElementKind::TAB_HEADER)
            finishTabDrag(state, *control);
    }
}
