#include "gran/input/input_router.hpp"

#include <algorithm>

namespace gran::input
{
    namespace
    {
        // Длина первой панели → хранимое положение: px, % от места или длина второй панели (fixed="second")
        // The first pane length → the stored position: px, % of the room or the second pane length (fixed="second")
        float storedPosition(const ui::SplitterState& split, float first) noexcept
        {
            if (split.percent && split.room > 0.0f)
                return first / split.room * 100.0f;
            if (split.fixed == ui::SplitFixed::SECOND)
                return split.room - first;
            return first;
        }
    }

    /*
    ** Новая длина первой панели в пределах ручки (minFirst … место − minSecond); свёрнутая панель разворачивается.
    ** onChange — не здесь, а по окончании перетаскивания и по клавише (finishSplit): обработчик не дёргается на каждый пиксель.
    ** true — положение изменилось.
    **
    ** A new first pane length within the handle limits (minFirst … room − minSecond); a collapsed pane expands.
    ** onChange is not here but at the drag end and on a key (finishSplit): the handler is not pulled on every pixel.
    ** true — the position changed.
    */
    bool setSplit(ui::Node& splitter, float first)
    {
        ui::Node* handle = ui::splitterHandle(splitter);
        ui::SplitterState& split = splitter.content.split;

        if (handle == nullptr)
            return false;
        first = std::clamp(first, handle->content.minimum, handle->content.maximum);
        if (first == handle->content.value && !split.collapsed)
            return false;
        split.position = storedPosition(split, first);
        split.collapsed = false;
        handle->content.value = first;
        ui::markDirty(splitter, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        return true;
    }

    /*
    ** Перетаскивание ручки: точка окна → координата внутри разделителя (с преобразованиями) за вычетом рамки,
    ** отступа и места, за которое ручку взяли (splitGrab), — ручка не прыгает под указатель.
    **
    ** Dragging the handle: the window point → a coordinate inside the splitter (with the transforms) minus the border,
    ** the padding and the place the handle was grabbed at (splitGrab) — the handle does not jump under the pointer.
    */
    void dragSplitter(RouterState& state, ui::Node& handle, Vec2 point)
    {
        ui::Node* splitter = handle.parent;

        if (splitter == nullptr)
            return;
        // windowToNode даёт точку в пространстве рамок (как у ползунка): начало содержимого — рамка + граница + отступ
        // windowToNode gives the point in the frame space (as for the slider): the content start is the frame + the border + the padding
        Vec2 local = ui::windowToNode(*splitter, point);
        const style::ComputedStyle& style = splitter->computed;
        const Rect& frame = splitter->layout.frame;
        if (style.orientation == static_cast<int>(style::Orientation::HORIZONTAL))
            setSplit(*splitter, local.x - frame.x - style.borderWidth.left - style.padding.left - state.splitGrab);
        else
            setSplit(*splitter, local.y - frame.y - style.borderWidth.top - style.padding.top - state.splitGrab);
    }

    /*
    ** Ручка в фокусе: стрелки — шаг 10 логических пикселей (с Shift — 50), Home и End — пределы, Enter — свернуть
    ** или развернуть. Изменение сразу сообщается onChange. true — клавиша для ручки.
    **
    ** The focused handle: the arrows — a step of 10 logical pixels (with Shift — 50), Home and End — the limits, Enter — collapse
    ** or expand. A change is reported to onChange at once. true — the key is for the handle.
    */
    bool splitterKey(RouterState& state, ui::Node& handle, const platform::Event& event)
    {
        float step = 10.0f;
        float target = handle.content.value;
        platform::Key key = event.key;

        if ((event.modifiers & platform::MODIFIER_SHIFT) != 0)
            step = 50.0f;
        if (key == platform::Key::ENTER && handle.parent != nullptr)
        {
            toggleSplit(state, *handle.parent);
            return true;
        }
        if (key == platform::Key::LEFT || key == platform::Key::UP)
            target -= step;
        else if (key == platform::Key::RIGHT || key == platform::Key::DOWN)
            target += step;
        else if (key == platform::Key::HOME)
            target = handle.content.minimum;
        else if (key == platform::Key::END)
            target = handle.content.maximum;
        else
            return false;
        if (handle.parent != nullptr && setSplit(*handle.parent, target))
            finishSplit(state, *handle.parent);
        return true;
    }

    // Свернуть или развернуть панель collapse (двойной щелчок по ручке, Enter); true — состояние сменилось
    // Collapsing or expanding the collapse pane (a double click on the handle, Enter); true — the state changed
    bool toggleSplit(RouterState& state, ui::Node& splitter)
    {
        if (splitter.content.split.collapse == ui::SplitCollapse::NONE)
            return false;
        splitter.content.split.collapsed = !splitter.content.split.collapsed;
        ui::markDirty(splitter, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        finishSplit(state, splitter);
        return true;
    }
}
