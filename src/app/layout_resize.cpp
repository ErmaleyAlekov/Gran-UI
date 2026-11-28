#include "gran/app/internal/app_state.hpp"
#include "gran/markup/customization.hpp"

#include <algorithm>
#include <cmath>

namespace gran::app
{
    namespace
    {
        // Возвращаем сторону ручки в логических координатах узла.
        // Return the handle side in the node's logical coordinates.
        float edge(float coordinate, float start, float length)
        {
            if (std::abs(coordinate - start) <= 6) return -1;
            if (std::abs(coordinate - start - length) <= 6) return 1;
            return 0;
        }

        // Размер привязывается к сетке внутри объявленных пределов компоновки.
        // Snap the size to the grid within the declared layout limits.
        std::string extent(float length, float minimum, float maximum)
        {
            const float snapped = std::round(length / 4.0f) * 4.0f;
            return std::to_string(static_cast<long long>(std::ceil(std::clamp(snapped, minimum, maximum))));
        }
    }

    // Обе координаты размера входят в одну отменяемую правку.
    // Both dimensions belong to a single undoable edit.
    bool resizeLayout(AppWindow& win, const platform::Event& event)
    {
        auto& state = win.shared;
        auto* node = state.editSelection.empty() ? nullptr : ui::findNode(*win.document.root, state.editSelection);
        if (!node || !node->domElement || !markup::permitsCustomization(*node->domElement, markup::Customization::STYLE))
            return false;
        const Rect frame = node->layout.frame;
        const auto pos = ui::windowToNode(*node, event.position);
        if (event.kind == platform::EventKind::MOUSE_DOWN)
            state.editGrip = {edge(pos.x, frame.x, frame.width), edge(pos.y, frame.y, frame.height)};
        if (!state.editGrip.x || !state.editGrip.y)
            return false;
        if (event.kind == platform::EventKind::MOUSE_UP)
        {
            const float w = state.editGrip.x > 0 ? pos.x - frame.x : frame.width + frame.x - pos.x;
            const float h = state.editGrip.y > 0 ? pos.y - frame.y : frame.height + frame.y - pos.y;
            state.editGrip = {};
            state.owner->editLayout("Resize", {{"id", node->id},
                {"width", extent(w, node->computed.minWidth, node->computed.maxWidth)},
                {"height", extent(h, node->computed.minHeight, node->computed.maxHeight)}});
        }
        return true;
    }
}
