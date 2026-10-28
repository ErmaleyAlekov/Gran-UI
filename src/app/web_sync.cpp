#include "gran/app/internal/app_state.hpp"

namespace gran::app
{
    namespace
    {
        // Отсекает ли узел своих детей по краю (прокрутка или overflow="hidden") / Whether the node clips its children at its edge (scrolling or overflow="hidden")
        bool clipsChildren(const ui::Node& node) noexcept
        {
            return node.kind == ui::ElementKind::SCROLL
                || node.computed.overflow == static_cast<int>(style::Overflow::HIDDEN);
        }

        /*
        ** Обход видимых узлов с областью отсечения предков. Скрытая ветвь (visible="false",
        ** закрытый Popup) не обходится — её WebView в этом проходе не отмечаются и будут скрыты.
        **
        ** A walk over the visible nodes with the clipping area of the ancestors. A hidden branch (visible="false",
        ** a closed Popup) is not walked — its WebViews are not marked in this pass and will be hidden.
        */
        void walk(AppWindow& win, ui::Node& node, const Rect& clip, std::uint32_t pass)
        {
            Rect childClip = clip;

            if (!node.visible)
                return;
            if (node.kind == ui::ElementKind::WEB_VIEW)
                placeWebView(win, node, clip, pass);
            if (clipsChildren(node))
                childClip = rectIntersect(clip, affineBounds(ui::nodeToWindow(node), node.layout.frame));
            for (const std::unique_ptr<ui::Node>& child : node.children)
                walk(win, *child, childClip, pass);
        }
    }

    // Слоты, не встреченные в проходе, скрываются (браузер живёт, но не рисуется) / The slots not met in the pass are hidden (the browser lives but is not drawn)
    void syncWebViews(AppWindow& win)
    {
        Size pixels = win.window->pixelSize();
        float scale = win.window->scale();
        Rect windowArea{0.0f, 0.0f, pixels.width / scale, pixels.height / scale};

        ++win.webPass;
        walk(win, *win.document.root, windowArea, win.webPass);
        for (const std::unique_ptr<WebSlot>& slot : win.webSlots)
        {
            if (slot->pass == win.webPass || !slot->shown || !slot->view)
                continue;
            slot->view->setVisible(false);
            slot->shown = false;
        }
    }
}
