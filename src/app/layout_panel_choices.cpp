#include "gran/app/internal/app_state.hpp"
#include "beresta.hpp"

namespace gran::app
{
    namespace
    {
        // Повторное обновление открытой панели сохраняет имеющиеся пункты.
        // Refreshing the open panel again preserves its existing entries.
        void addOption(ui::Node& combo, const std::string& id, const std::string& label)
        {
            auto item = std::make_unique<ui::Node>();
            item->kind = ui::ElementKind::MENU_ITEM;
            item->content.text = label;
            item->content.option = id;
            auto& list = *ui::comboList(combo);
            ui::appendChild(list, std::move(item));
            ui::markDirty(list, ui::DIRTY_STYLE | ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        }

        // Удаление через приложение освобождает ссылки ввода на исчезнувшие пункты.
        // Removing through the application releases input references to vanished entries.
        void updateChoices(Application& app, ui::Node& combo, markup::TokenMap choices)
        {
            auto& list = *ui::comboList(combo);
            for (std::size_t index = list.children.size(); index > 0; --index)
            {
                auto& item = *list.children[index - 1];
                if (item.content.option != "-" && !choices.erase(item.content.option)) app.remove(item);
            }
            for (const auto& [id, label] : choices) addOption(combo, id, label);
            app.selectOption(combo, combo.content.option);
        }
    }

    // После фиксации профиля новые имена доступны без переоткрытия панели.
    // After commit, new names become available without reopening the panel.
    void refreshLayoutChoices(Application::State& state)
    {
        if (!state.layoutPanel || state.windows.empty())
            return;
        auto& saved = *ui::findNode(*state.layoutPanel, "saved");
        markup::TokenMap choices;
        for (const auto& name : state.owner->profiles())
            choices.emplace(name, name);
        updateChoices(*state.owner, saved, std::move(choices));
        state.owner->selectOption(saved, state.currentProfile.name);
    }
}
