#include "gran/render/internal/scene_internal.hpp"

#include <algorithm>

namespace gran::render::internal
{
    namespace
    {
        // Сравнение по zIndex для устойчивой сортировки / A zIndex comparison for the stable sort
        struct ZIndexLess
        {
            bool operator()(const ui::Node* first, const ui::Node* second) const noexcept
            {
                return first->computed.zIndex < second->computed.zIndex;
            }
        };

        // Порядок детей: по zIndex с сохранением порядка при равенстве (как в CSS)
        // The children order: by zIndex keeping the order on a tie (as in CSS)
        std::vector<const ui::Node*> paintOrder(const ui::Node& node)
        {
            std::vector<const ui::Node*> order;
            bool layered = false;

            order.reserve(node.children.size());
            for (const std::unique_ptr<ui::Node>& child : node.children)
            {
                order.push_back(child.get());
                layered = layered || child->computed.zIndex != 0;
            }
            if (layered)
                std::stable_sort(order.begin(), order.end(), ZIndexLess{});
            return order;
        }

        /*
        ** Собственное содержимое узла: элемент управления, картинка или текст. Текст рисует лист,
        ** Text с участками, а также пункт меню и ComboBox — их единственный ребёнок-слой (подменю,
        ** список) в поток не входит; стрелка — поверх подписи.
        **
        ** The node own content: a control, an image or a text. The text is painted by a leaf,
        ** a Text with runs, and also a menu item and a ComboBox — their only child layer (a submenu,
        ** a list) is out of the flow; the arrow goes over the label.
        */
        void paintContent(const ui::Node& node, const PaintState& inner, SceneContext& context)
        {
            ui::ElementKind kind = node.kind;
            bool marked = kind == ui::ElementKind::MENU_ITEM || kind == ui::ElementKind::COMBO_BOX;

            if (kind == ui::ElementKind::CHECKBOX || kind == ui::ElementKind::SLIDER || kind == ui::ElementKind::RADIO_BUTTON
                || kind == ui::ElementKind::SWITCH || kind == ui::ElementKind::PROGRESS_BAR)
                paintControl(node, inner, context);
            else if (kind == ui::ElementKind::IMAGE)
                paintImage(node, inner, context);
            else if (kind == ui::ElementKind::ICON)
                paintIcon(node, inner, context);
            // Подпись страницы Tab рисует её заголовок, а не страница / The Tab page label is painted by its header, not by the page
            else if ((node.children.empty() && kind != ui::ElementKind::TAB) || ui::hasSpans(node) || marked)
                paintText(node, inner, context);
            if (marked)
                paintMark(node, inner, context);
        }
    }

    // Невидимые и полностью прозрачные ветви пропускаются целиком; содержимое (текст, дети) — внутри отсечения узла
    // Invisible and fully transparent branches are skipped entirely; the content (text, children) is inside the node clip
    void paintNode(const ui::Node& node, const PaintState& parent, SceneContext& context)
    {
        // Участки (Span) рисует их Text: у участка нет своего блока; слои рисуются после всего дерева
        // The runs (Span) are painted by their Text: a run has no box of its own; the layers are painted after the whole tree
        if (!node.visible || node.computed.opacity <= 0.0f || node.kind == ui::ElementKind::SPAN)
            return;
        if (ui::isLayer(node) && &node != context.layer)
            return;
        PaintState state = nodeState(node, parent);
        paintBox(node, state, context);
        PaintState inner = clipChildren(node, state, context);
        paintContent(node, inner, context);
        for (const ui::Node* child : paintOrder(node))
            paintNode(*child, inner, context);
        if (node.kind == ui::ElementKind::SCROLL || node.kind == ui::ElementKind::TEXT_AREA)
            paintScrollbars(node, inner, context);
    }

}

namespace gran::render
{
    /*
    ** Отсечение 0 — «без отсечения» (шейдер его не читает, но слот обязан существовать).
    ** Фон окна — цвет очистки; градиентный фон — прямоугольник на всё окно.
    **
    ** Clip 0 is "no clip" (the shader does not read it, but the slot must exist).
    ** The window background is the clear color; a gradient background is a rectangle over the whole window.
    ** Первый сегмент открыт с начала; подложки открывают следующие (paint_backdrop.cpp).
    ** The first segment is open from the start; backdrops open the next ones (paint_backdrop.cpp).
    */
    void buildScene(const ui::Node& root, const style::ComputedStyle& window, float scale, SceneResources& resources,
        Scene& scene)
    {
        internal::SceneContext context{resources, scene, scale, nullptr, Rect{}};
        internal::PaintState state;
        const Rect& frame = root.layout.frame;

        // Окно — рамка корня вместе с его внешними отступами / The window is the root frame together with its margins
        context.viewport = Rect{0.0f, 0.0f, frame.x + frame.width + root.computed.margin.right,
            frame.y + frame.height + root.computed.margin.bottom};

        scene.instances.clear();
        scene.clips.assign(1, GpuClip{});
        scene.segments.clear();
        scene.clearColor = Color{1.0f, 1.0f, 1.0f, 1.0f};
        state.transform = Affine2D{scale, 0.0f, 0.0f, scale, 0.0f, 0.0f};
        if (window.background.kind == style::PaintKind::COLOR)
            scene.clearColor = window.background.color;
        scene.segments.push_back(rhi::FrameSegment{0, 0, false, 0.0f});
        internal::paintNode(root, state, context);
        internal::paintPopups(root, state, context);
        internal::closeSegment(scene);
    }
}
