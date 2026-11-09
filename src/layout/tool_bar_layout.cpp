#include "gran/layout/internal/layout_internal.hpp"

#include <limits>
#include <vector>

namespace gran::layout::internal
{
    namespace
    {
        /*
        ** Отдать ребёнка панели в меню «ещё» (скрыть) или вернуть его. Скрытый так узел помечен overflowed:
        ** видимость, которую задало приложение (visible="false"), панель не трогает и не восстанавливает.
        **
        ** Giving a toolbar child to the "more" menu (hiding it) or returning it. A node hidden this way is marked overflowed:
        ** the visibility set by the application (visible="false") is neither touched nor restored by the toolbar.
        */
        void setOverflowed(ui::Node& child, bool overflowed) noexcept
        {
            if (child.content.overflowed == overflowed)
                return;
            child.content.overflowed = overflowed;
            child.visible = !overflowed;
            child.dirty = static_cast<std::uint8_t>(child.dirty | ui::DIRTY_PAINT);
        }

        // Длина ребёнка по главной оси с внешними отступами / The child length along the main axis with the margins
        float mainLength(ui::Node& child, Size available, bool horizontal, LayoutPass& pass)
        {
            Size size = measureNode(child, available, pass);

            if (horizontal)
                return size.width + child.computed.margin.left + child.computed.margin.right;
            return size.height + child.computed.margin.top + child.computed.margin.bottom;
        }

        // Ребёнок, которого панель раскладывает: не кнопка «ещё» и не скрытый приложением / A child the toolbar lays out: not the "more" button and not hidden by the application
        bool fitsCandidate(const ui::Node& child) noexcept
        {
            return child.kind != ui::ElementKind::TOOL_MORE && (child.visible || child.content.overflowed) && !ui::isLayer(child);
        }

        /*
        ** Раздача места room по порядку детей: never остаются всегда, always уходят всегда, остальные остаются,
        ** пока помещаются; после первого ушедшего уходят и все следующие — порядок кнопок в меню тот же, что в панели.
        **
        ** Handing out the room in the children order: never always stay, always always go, the others stay
        ** while they fit; after the first one that went, all the next ones go too — the button order in the menu is the same as in the bar.
        */
        void placeOverflow(ui::Node& bar, float room, const std::vector<float>& lengths)
        {
            std::size_t index = 0;
            float used = 0.0f;
            bool spilled = false;

            for (const std::unique_ptr<ui::Node>& child : bar.children)
            {
                if (!fitsCandidate(*child))
                    continue;
                float length = lengths[index++] + bar.computed.spacing;
                ui::OverflowMode mode = child->content.overflowMode;
                bool stays = mode == ui::OverflowMode::NEVER || (mode == ui::OverflowMode::AS_NEEDED && !spilled
                    && used + length - bar.computed.spacing <= room);
                spilled = spilled || (!stays && mode == ui::OverflowMode::AS_NEEDED);
                if (stays)
                    used += length;
                setOverflowed(*child, !stays);
            }
        }
    }

    /*
    ** Панель инструментов перед размещением стеком: дети идут по порядку, пока помещаются; остальные (и все
    ** с overflowMode="always") уходят в меню кнопки «ещё», которая тогда занимает место в конце. Дети с
    ** overflowMode="never" остаются всегда. Всё поместилось — кнопки «ещё» нет.
    **
    ** The toolbar before the stack placement: the children go in order while they fit; the rest (and all
    ** with overflowMode="always") go into the menu of the "more" button, which then takes a place at the end. The children with
    ** overflowMode="never" always stay. Everything fits — there is no "more" button.
    */
    void fitToolBar(ui::Node& bar, Rect content, LayoutPass& pass)
    {
        bool horizontal = bar.computed.orientation == static_cast<int>(style::Orientation::HORIZONTAL);
        float limit = mainOf(Size{content.width, content.height}, horizontal);
        float infinite = std::numeric_limits<float>::infinity();
        Size available = sizeFrom(infinite, crossOf(Size{content.width, content.height}, horizontal), horizontal);
        ui::Node* more = ui::toolBarMore(bar);
        std::vector<float> lengths;
        float total = 0.0f;
        bool forced = false;

        for (const std::unique_ptr<ui::Node>& child : bar.children)
        {
            if (!fitsCandidate(*child))
                continue;
            setOverflowed(*child, false);
            lengths.push_back(mainLength(*child, available, horizontal, pass));
            total += lengths.back() + bar.computed.spacing;
            forced = forced || child->content.overflowMode == ui::OverflowMode::ALWAYS;
        }
        if (more == nullptr)
            return;
        bool overflow = forced || total - bar.computed.spacing > limit;
        more->visible = overflow;
        if (overflow)
            placeOverflow(bar, limit - mainLength(*more, available, horizontal, pass) - bar.computed.spacing, lengths);
    }
}
