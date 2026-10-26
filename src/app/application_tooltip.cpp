#include "gran/app/internal/app_state.hpp"

#include <algorithm>
#include <memory>

namespace gran::app
{
    namespace
    {
        // Задержка подсказки — как у title в браузерах / The tooltip delay — as for title in browsers
        constexpr double TOOLTIP_DELAY = 0.6;

        // Высота указателя мыши: подсказка встаёт под ним, а не под острием / The mouse pointer height: the tooltip stands under it, not under its tip
        constexpr float POINTER_HEIGHT = 18.0f;

        // Ближайший узел с атрибутом tooltip среди узла под мышью и его предков / The nearest node with the tooltip attribute among the node under the mouse and its ancestors
        const ui::Node* tooltipSource(const ui::Node* node) noexcept
        {
            for (const ui::Node* current = node; current != nullptr; current = current->parent)
            {
                if (!current->content.tooltip.empty())
                    return current;
            }
            return nullptr;
        }
    }

    // Стиль Tooltip берётся из темы по имени: у слоя подсказки нет своего элемента разметки / The Tooltip style is taken from the theme by name: the tooltip layer has no markup element of its own
    void createTooltip(Application::State& state)
    {
        std::unique_ptr<ui::Node> popup = std::make_unique<ui::Node>();
        std::unique_ptr<ui::Node> text = std::make_unique<ui::Node>();

        popup->kind = ui::ElementKind::POPUP;
        popup->visible = false;
        popup->rule = markup::defaultTheme().document.styles.find("Tooltip");
        text->kind = ui::ElementKind::TEXT;
        state.tooltipText = &ui::appendChild(*popup, std::move(text));
        state.tooltip = &ui::appendChild(*state.document.root, std::move(popup));
        ui::markDirty(*state.tooltip, ui::DIRTY_STYLE | ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
    }

    // Нажатие мыши, клавиша или колесо прячут подсказку до перехода на другой узел (как в Windows)
    // A mouse press, a key or the wheel hide the tooltip until moving to another node (as in Windows)
    void hideTooltip(Application::State& state, bool suppress)
    {
        if (state.tooltipShown && state.router)
            state.router->closePopup(*state.tooltip);
        state.tooltipShown = false;
        state.tooltipSuppressed = state.tooltipSuppressed || suppress;
    }

    /*
    ** Подсказка появляется, когда мышь простояла TOOLTIP_DELAY над узлом с tooltip; встаёт
    ** под указателем (пассивный слой: мышь проходит сквозь него). Переход на другой узел
    ** прячет её и запускает отсчёт заново.
    **
    ** The tooltip appears when the mouse has stood TOOLTIP_DELAY over a node with tooltip; it stands
    ** under the pointer (a passive layer: the mouse goes through it). Moving to another node
    ** hides it and restarts the countdown.
    */
    bool updateTooltip(Application::State& state, double time)
    {
        const ui::Node* source = tooltipSource(state.router->hovered());
        bool shown = state.tooltipShown;

        if (source != state.tooltipSource)
        {
            hideTooltip(state, false);
            state.tooltipSource = source;
            state.tooltipTime = time + TOOLTIP_DELAY;
            state.tooltipSuppressed = false;
            return shown;
        }
        if (source == nullptr || shown || state.tooltipSuppressed || time < state.tooltipTime)
            return false;
        state.tooltipText->content.text = source->content.tooltip;
        ui::markDirty(*state.tooltipText, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        Vec2 pointer = state.router->pointer();
        state.router->openPopup(*state.tooltip, Rect{pointer.x, pointer.y, 0.0f, POINTER_HEIGHT}, true);
        state.tooltipShown = true;
        return true;
    }

    // Ждать есть чего, только пока подсказка ещё не показана и не подавлена / There is something to wait for only while the tooltip is not shown and not suppressed yet
    double tooltipWait(const Application::State& state, double time) noexcept
    {
        if (state.tooltipSource == nullptr || state.tooltipShown || state.tooltipSuppressed)
            return -1.0;
        return std::max(0.0, state.tooltipTime - time);
    }
}
