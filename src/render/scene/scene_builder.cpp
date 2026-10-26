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
    }

    // Невидимые и полностью прозрачные ветви пропускаются целиком; содержимое (текст, дети) — внутри отсечения узла
    // Invisible and fully transparent branches are skipped entirely; the content (text, children) is inside the node clip
    void paintNode(const ui::Node& node, const PaintState& parent, SceneContext& context)
    {
        // Участки (Span) рисует их Text: у участка нет своего блока; слои Popup рисуются после всего дерева
        // The runs (Span) are painted by their Text: a run has no box of its own; Popup layers are painted after the whole tree
        if (!node.visible || node.computed.opacity <= 0.0f || node.kind == ui::ElementKind::SPAN)
            return;
        if (node.kind == ui::ElementKind::POPUP && &node != context.layer)
            return;
        PaintState state = nodeState(node, parent);
        paintBox(node, state, context);
        PaintState inner = clipChildren(node, state, context);
        if (node.kind == ui::ElementKind::CHECKBOX || node.kind == ui::ElementKind::SLIDER)
            paintControl(node, inner, context);
        else if (node.kind == ui::ElementKind::IMAGE)
            paintImage(node, inner, context);
        else if (node.children.empty() || ui::hasSpans(node))
            paintText(node, inner, context);
        for (const ui::Node* child : paintOrder(node))
            paintNode(*child, inner, context);
        if (node.kind == ui::ElementKind::SCROLL || node.kind == ui::ElementKind::TEXT_AREA)
            paintScrollbars(node, inner, context);
    }

    /*
    ** Открытые слои поверх всего дерева в порядке дерева: без отсечений и преобразований предков
    ** (слой живёт в координатах окна), со своим преобразованием и прозрачностью.
    **
    ** The open layers over the whole tree in tree order: without the ancestor clips and transforms
    ** (a layer lives in window coordinates), with its own transform and opacity.
    */
    void paintPopups(const ui::Node& node, const PaintState& base, SceneContext& context)
    {
        for (const std::unique_ptr<ui::Node>& child : node.children)
        {
            if (!child->visible)
                continue;
            if (child->kind != ui::ElementKind::POPUP)
            {
                paintPopups(*child, base, context);
                continue;
            }
            context.layer = child.get();
            paintNode(*child, base, context);
            context.layer = nullptr;
        }
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
        internal::SceneContext context{resources, scene, scale};
        internal::PaintState state;

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
