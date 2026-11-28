#include "gran/markup/internal/overlay.hpp"
#include "gran/markup/theme.hpp"
#include "gran/a11y/access_tree.hpp"
#include "gran/style/value_parsers.hpp"
#include "gran/ui/style_pass.hpp"

#include <algorithm>
#include <cmath>

namespace gran::markup::internal
{
    namespace
    {
        struct Limits
        {
            float size = 0;
            float contrast = 0;
            bool name = false;
            float opacity = 1;
            DesignBackdrop backdrop;
        };

        // Явное значение заменяет предел области у родителя.
        // An explicit value replaces the limit inherited from the parent region.
        void readLimits(const beresta::Element& el, Limits& limits)
        {
            if (const auto* size = findAttribute(el, "minTargetSize")) limits.size = style::parseNumber(*size);
            if (const auto* ratio = findAttribute(el, "minTextContrast")) limits.contrast = style::parseNumber(*ratio);
            if (const auto* name = findAttribute(el, "requireAccessibleName")) limits.name = style::parseBoolean(*name);
        }

        // Передаём потомкам вычисленный фон и авторские пределы области.
        // Pass the resolved backdrop and the author's regional limits to descendants.
        void inspect(ui::Node& node, Limits limits)
        {
            if (!node.visible) return;
            if (node.domElement) readLimits(*node.domElement, limits);
            limits.opacity *= node.computed.opacity;
            designBackdrop(node.computed.background, limits.opacity, limits.backdrop);
            if (node.domElement && a11y::isFocusable(node))
            {
                if (limits.name && a11y::accessibleName(node).empty())
                    denyOverlay(*node.domElement, "requireAccessibleName");
            }
            if (limits.contrast > 1 && node.domElement
                && (!node.content.text.empty() || !node.content.placeholder.empty() || ui::isTextField(node)))
            {
                if (designContrast(node.computed.color, limits.opacity, limits.backdrop) < limits.contrast)
                    denyOverlay(*node.domElement, "minTextContrast");
            }
            for (auto& child : node.children) inspect(*child, limits);
        }
    }

    // Проверка использует тот же каскад, что и обычная отрисовка окна.
    // Validation uses the same cascade as normal window rendering.
    void checkOverlayDesign(UiDocument& document)
    {
        Limits limits;
        readLimits(*document.dom->getRoot(), limits);
        TargetTransforms transforms;
        inspectDesignStates(document, [&](ui::Node& root, bool collectTransforms)
        {
            const auto& theme = defaultTheme();
            auto style = windowStyle(theme, document.window);
            auto local = limits;
            designBackdrop(style.background, 1, local.backdrop);
            ui::markDirty(root, ui::DIRTY_STYLE);
            ui::resolveStyles(root, style, theme.kinds, nullptr);
            if (collectTransforms) recordTargetTransform(root, transforms);
            inspect(root, local);
        });
        if (!transforms.empty()) checkTargetSizes(*document.root, limits.size, transforms);
    }
}
