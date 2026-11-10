#include "gran/layout/internal/layout_internal.hpp"

#include <algorithm>
#include <cmath>

namespace gran::layout::internal
{
    namespace
    {
        // Панель, скрытая разделителем (свёрнута), — не приложением: флаг overflowed, как у панели инструментов
        // A pane hidden by the splitter (collapsed), not by the application: the overflowed flag, as for the toolbar
        void setPaneHidden(ui::Node& pane, bool hidden) noexcept
        {
            if (pane.content.overflowed == hidden || (!pane.visible && !pane.content.overflowed))
                return;
            pane.content.overflowed = hidden;
            pane.visible = !hidden;
            pane.dirty = static_cast<std::uint8_t>(pane.dirty | ui::DIRTY_PAINT);
        }

        /*
        ** Длина первой панели по главной оси: положение (px, % от места или длина второй панели при fixed="second"),
        ** без положения — половина; затем пределы minFirst и minSecond (не хватает места на оба — важнее minFirst).
        **
        ** The first pane length along the main axis: the position (px, % of the room or the second pane length with fixed="second"),
        ** without a position — a half; then the minFirst and minSecond limits (no room for both — minFirst matters more).
        */
        float firstLength(const ui::SplitterState& split, float room) noexcept
        {
            float first = room * 0.5f;

            if (split.position >= 0.0f && split.percent)
                first = room * split.position / 100.0f;
            else if (split.position >= 0.0f)
                first = split.position;
            if (split.position >= 0.0f && split.fixed == ui::SplitFixed::SECOND)
                first = room - first;
            first = std::min(first, room - split.minSecond);
            return std::round(std::max(first, std::min(split.minFirst, room)));
        }

        // Длины по главной оси: первая панель, ручка; ось разделителя / The main axis lengths: the first pane, the handle; the splitter axis
        struct SplitPlace
        {
            float first;
            float thickness;
            bool horizontal;
        };

        // Отрезок прямоугольника по главной оси: начало и длина / A rectangle segment along the main axis: the start and the length
        Rect segment(Rect content, float start, float length, bool horizontal) noexcept
        {
            if (horizontal)
            {
                content.x += start;
                content.width = length;
                return content;
            }
            content.y += start;
            content.height = length;
            return content;
        }

        // Рамки трёх детей: первая панель, ручка, вторая панель — по поперечной оси во всю высоту (ширину)
        // The three children frames: the first pane, the handle, the second pane — across the full cross size
        void placeSplit(ui::Node& splitter, Rect content, const SplitPlace& place, LayoutPass& pass)
        {
            float total = mainOf(Size{content.width, content.height}, place.horizontal);
            float rest = std::max(0.0f, total - place.first - place.thickness);

            if (splitter.children[0]->visible)
                arrangeNode(*splitter.children[0], segment(content, 0.0f, place.first, place.horizontal), pass);
            arrangeNode(*splitter.children[1], segment(content, place.first, place.thickness, place.horizontal), pass);
            if (splitter.children[2]->visible)
                arrangeNode(*splitter.children[2], segment(content, place.first + place.thickness, rest, place.horizontal), pass);
        }
    }

    /*
    ** Разделитель: [первая панель][ручка][вторая панель] по главной оси. Свёрнутая панель скрыта, её место отдано
    ** соседке. Пределы положения пишутся в ручку (content.minimum/maximum/value) — их читают клавиши и UI Automation;
    ** место под панели (split.room) — для перевода длины обратно в положение (px, %, fixed="second").
    **
    ** A splitter: [the first pane][the handle][the second pane] along the main axis. A collapsed pane is hidden, its room is given
    ** to the neighbor. The position limits are written into the handle (content.minimum/maximum/value) — the keys and UI Automation read them;
    ** the room for the panes (split.room) — to convert a length back into the position (px, %, fixed="second").
    */
    void arrangeSplitter(ui::Node& splitter, Rect content, LayoutPass& pass)
    {
        bool horizontal = splitter.computed.orientation == static_cast<int>(style::Orientation::HORIZONTAL);
        ui::Node* first = ui::splitterPane(splitter, 0);
        ui::Node* handle = ui::splitterHandle(splitter);
        ui::Node* second = ui::splitterPane(splitter, 1);
        ui::SplitterState& split = splitter.content.split;

        if (first == nullptr || handle == nullptr || second == nullptr)
            return;
        float thickness = mainOf(measureNode(*handle, Size{content.width, content.height}, pass), horizontal);
        float room = std::max(0.0f, mainOf(Size{content.width, content.height}, horizontal) - thickness);
        float length = firstLength(split, room);
        split.room = room;
        setPaneHidden(*first, split.collapsed && split.collapse == ui::SplitCollapse::FIRST);
        setPaneHidden(*second, split.collapsed && split.collapse == ui::SplitCollapse::SECOND);
        if (!first->visible)
            length = 0.0f;
        if (!second->visible)
            length = room;
        handle->content.minimum = std::min(split.minFirst, room);
        handle->content.maximum = std::max(handle->content.minimum, room - split.minSecond);
        handle->content.value = length;
        placeSplit(splitter, content, SplitPlace{length, thickness, horizontal}, pass);
    }
}
