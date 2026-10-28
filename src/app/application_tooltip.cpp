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
    void createTooltip(AppWindow& win)
    {
        std::unique_ptr<ui::Node> popup = std::make_unique<ui::Node>();
        std::unique_ptr<ui::Node> text = std::make_unique<ui::Node>();

        popup->kind = ui::ElementKind::POPUP;
        popup->visible = false;
        popup->rule = markup::defaultTheme().document.styles.find("Tooltip");
        text->kind = ui::ElementKind::TEXT;
        win.tooltipText = &ui::appendChild(*popup, std::move(text));
        win.tooltip = &ui::appendChild(*win.document.root, std::move(popup));
        ui::markDirty(*win.tooltip, ui::DIRTY_STYLE | ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
    }

    // Нажатие мыши, клавиша или колесо прячут подсказку до перехода на другой узел (как в Windows)
    // A mouse press, a key or the wheel hide the tooltip until moving to another node (as in Windows)
    void hideTooltip(AppWindow& win, bool suppress)
    {
        if (win.tooltipShown && win.router)
            win.router->closePopup(*win.tooltip);
        win.tooltipShown = false;
        win.tooltipSuppressed = win.tooltipSuppressed || suppress;
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
    bool updateTooltip(AppWindow& win, double time)
    {
        const ui::Node* source = tooltipSource(win.router->hovered());
        bool shown = win.tooltipShown;

        if (source != win.tooltipSource)
        {
            hideTooltip(win, false);
            win.tooltipSource = source;
            win.tooltipTime = time + TOOLTIP_DELAY;
            win.tooltipSuppressed = false;
            return shown;
        }
        if (source == nullptr || shown || win.tooltipSuppressed || time < win.tooltipTime)
            return false;
        win.tooltipText->content.text = source->content.tooltip;
        ui::markDirty(*win.tooltipText, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        Vec2 pointer = win.router->pointer();
        win.router->openPopup(*win.tooltip, Rect{pointer.x, pointer.y, 0.0f, POINTER_HEIGHT}, true);
        win.tooltipShown = true;
        return true;
    }

    // Ждать есть чего, только пока подсказка ещё не показана и не подавлена / There is something to wait for only while the tooltip is not shown and not suppressed yet
    double tooltipWait(const AppWindow& win, double time) noexcept
    {
        if (win.tooltipSource == nullptr || win.tooltipShown || win.tooltipSuppressed)
            return -1.0;
        return std::max(0.0, win.tooltipTime - time);
    }
}
