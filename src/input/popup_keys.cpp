#include "gran/input/input_router.hpp"

#include <vector>

namespace gran::input
{
    namespace
    {
        // Верхний непассивный слой (подсказки клавиш не ловят) или nullptr / The top non-passive layer (tooltips do not catch keys) or nullptr
        ui::Node* topLayer(const RouterState& state) noexcept
        {
            for (std::size_t index = state.popups.size(); index > 0; --index)
            {
                if (!state.popups[index - 1]->content.popupPassive)
                    return state.popups[index - 1];
            }
            return nullptr;
        }

        /*
        ** Следующий включённый пункт слоя по кругу на шаг step от пункта в фокусе; если фокус
        ** вне слоя — первый (вниз) или последний (вверх) пункт, как в меню Windows.
        **
        ** The next enabled item of the layer cyclically by step from the focused item; if the focus
        ** is outside the layer — the first (down) or the last (up) item, as in Windows menus.
        */
        ui::Node* stepItem(ui::Node& popup, const ui::Node* current, int step)
        {
            std::vector<ui::Node*> items;
            long position = -1;

            for (const std::unique_ptr<ui::Node>& child : popup.children)
            {
                if (child->kind == ui::ElementKind::MENU_ITEM && child->visible && child->enabled)
                    items.push_back(child.get());
                if (child.get() == current)
                    position = static_cast<long>(items.size()) - 1;
            }
            if (items.empty())
                return nullptr;
            long count = static_cast<long>(items.size());
            if (position < 0 && step > 0)
                return items.front();
            if (position < 0)
                return items.back();
            return items[static_cast<std::size_t>((position + step + count) % count)];
        }
    }

    // Enter и пробел на пункте в фокусе обрабатывает общая активация (activate → chooseItem)
    // Enter and space on the focused item are handled by the common activation (activate → chooseItem)
    bool popupKey(RouterState& state, const platform::Event& event)
    {
        ui::Node* popup = topLayer(state);
        int step = 1;

        if (popup == nullptr)
            return false;
        if (event.key == platform::Key::ESCAPE)
        {
            closeLayer(state, *popup);
            return true;
        }
        if (event.key != platform::Key::UP && event.key != platform::Key::DOWN)
            return false;
        if (event.key == platform::Key::UP)
            step = -1;
        ui::Node* item = stepItem(*popup, state.focus, step);
        if (item != nullptr)
            changeFocus(state, item);
        return true;
    }
}
