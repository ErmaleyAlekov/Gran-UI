#include "gran/input/input_router.hpp"

#include <cstddef>

namespace gran::input
{
    namespace
    {
        /*
        ** Выбор члена группы: радиокнопка — onChange у неё самой (как событие change у отмеченного
        ** <input type="radio">), заголовок вкладки — смена страницы и onChange у Tabs.
        **
        ** Choosing a group member: a radio button — onChange of the button itself (as the change event of a checked
        ** <input type="radio">), a tab header — a page change and onChange of Tabs.
        */
        void selectMember(RouterState& state, ui::Node& member)
        {
            ui::Node* tabs = ui::tabsOf(member);
            ui::Node* target = &member;

            if (tabs != nullptr)
            {
                if (!ui::selectTab(*tabs, ui::optionValue(member)))
                    return;
                target = tabs;
            }
            else if (!ui::selectInGroup(member))
                return;
            if (!target->content.onChange.empty())
                state.actions->action(target->content.onChange, *target);
        }

        // Шаг стрелки в группе: 0 — не стрелка; у вкладок (полоса горизонтальна) — только влево и вправо
        // The arrow step in a group: 0 — not an arrow; for tabs (the strip is horizontal) — only left and right
        int arrowStep(const ui::Node& member, platform::Key key) noexcept
        {
            bool vertical = member.kind == ui::ElementKind::RADIO_BUTTON;

            if (key == platform::Key::RIGHT || (vertical && key == platform::Key::DOWN))
                return 1;
            if (key == platform::Key::LEFT || (vertical && key == platform::Key::UP))
                return -1;
            return 0;
        }

        // Следующий по кругу видимый и включённый член группы (nullptr — других нет)
        // The next visible and enabled group member around the circle (nullptr — there are no others)
        ui::Node* neighbor(const std::vector<ui::Node*>& group, const ui::Node& member, int step)
        {
            std::size_t count = group.size();
            std::size_t position = 0;

            while (position < count && group[position] != &member)
                ++position;
            for (std::size_t walked = 1; position < count && walked < count; ++walked)
            {
                std::size_t index = (position + walked) % count;
                // Назад: walked < count, поэтому сумма не уходит в минус / Backwards: walked < count, so the sum does not go negative
                if (step < 0)
                    index = (position + count - walked) % count;
                if (ui::isShown(*group[index]) && !disabled(group[index]))
                    return group[index];
            }
            return nullptr;
        }
    }

    // Флажок и переключатель меняют отметку, радиокнопка и заголовок вкладки выбирают себя
    // A check box and a switch toggle their mark, a radio button and a tab header choose themselves
    void changeChoice(RouterState& state, ui::Node& control)
    {
        if (control.kind == ui::ElementKind::RADIO_BUTTON || control.kind == ui::ElementKind::TAB_HEADER)
        {
            selectMember(state, control);
            return;
        }
        if (control.kind != ui::ElementKind::CHECKBOX && control.kind != ui::ElementKind::SWITCH)
            return;
        ui::setChecked(control, !control.content.checked);
        if (!control.content.onChange.empty())
            state.actions->action(control.content.onChange, control);
    }

    // Стрелка переносит фокус и выбор вместе (как у радиокнопок и вкладок в браузерах)
    // An arrow moves the focus and the choice together (as for radio buttons and tabs in browsers)
    bool stepChoice(RouterState& state, ui::Node& member, platform::Key key)
    {
        int step = arrowStep(member, key);

        if (step == 0)
            return false;
        std::vector<ui::Node*> group = ui::choiceGroup(member);
        if (group.empty())
            return false;
        ui::Node* next = neighbor(group, member, step);
        if (next == nullptr)
            return true;
        changeFocus(state, next);
        selectMember(state, *next);
        return true;
    }
}
