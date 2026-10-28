#include "gran/app/internal/app_state.hpp"

namespace gran::app
{
    namespace
    {
        // Блок содержимого в окне: рамка без рамки-обводки и отступов (обводка WebView видна вокруг страницы)
        // The content box in the window: the frame without the border and padding (the WebView border stays visible around the page)
        Rect contentRect(const ui::Node& node) noexcept
        {
            const style::ComputedStyle& style = node.computed;
            Rect box = node.layout.frame;

            box.x += style.borderWidth.left + style.padding.left;
            box.y += style.borderWidth.top + style.padding.top;
            box.width -= style.borderWidth.left + style.borderWidth.right + style.padding.left + style.padding.right;
            box.height -= style.borderWidth.top + style.borderWidth.bottom + style.padding.top + style.padding.bottom;
            return affineBounds(ui::nodeToWindow(node), box);
        }

        // Логические пиксели → физические / Logical pixels → physical ones
        Rect scaled(const Rect& rect, float scale) noexcept
        {
            return Rect{rect.x * scale, rect.y * scale, rect.width * scale, rect.height * scale};
        }

        bool sameRect(const Rect& first, const Rect& second) noexcept
        {
            return first.x == second.x && first.y == second.y && first.width == second.width
                && first.height == second.height;
        }
    }

    /*
    ** Браузер создаётся при первом показе узла (один раз: отказ не повторяется каждый кадр);
    ** границы и видимость передаются только при изменении — кадр без движения WebView
    ** не обращается к браузеру вовсе.
    **
    ** The browser is created at the first time the node is shown (once: a refusal is not repeated every frame);
    ** the bounds and visibility are passed only on a change — a frame without WebView movement
    ** does not call the browser at all.
    */
    void placeWebView(AppWindow& win, ui::Node& node, const Rect& clip, std::uint32_t pass)
    {
        WebSlot& slot = webSlot(win, node);
        float scale = win.window->scale();
        Rect content = contentRect(node);
        Rect full = scaled(content, scale);
        Rect visible = scaled(rectIntersect(content, clip), scale);

        slot.pass = pass;
        if (!slot.created)
            openWebView(win, slot);
        if (!slot.view)
            return;
        if (!sameRect(full, slot.full) || !sameRect(visible, slot.visible))
            slot.view->setBounds(full, visible);
        slot.full = full;
        slot.visible = visible;
        if (!slot.shown)
            slot.view->setVisible(true);
        slot.shown = true;
        sendContent(win, slot);
    }
}
