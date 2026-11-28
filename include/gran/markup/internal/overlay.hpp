#ifndef GRAN_MARKUP_INTERNAL_OVERLAY_HPP
#define GRAN_MARKUP_INTERNAL_OVERLAY_HPP

#include "gran/markup/customization.hpp"
#include "gran/markup/internal/build_context.hpp"
#include "gran/markup/overlay.hpp"
#include "beresta.hpp"

#include <set>

namespace gran::markup::internal
{
    struct OverlayContext
    {
        UiDocument& candidate;
        std::map<std::string, beresta::Element*, std::less<>> targets;
        std::vector<std::string>& missingIds;
        std::map<const beresta::Element*, TokenMap> scoped;
    };

    beresta::Element* overlayTarget(OverlayContext& context, const beresta::Element& operation, std::string_view key);
    void requirePermission(const beresta::Element& target, Customization permission);
    void collectMarkupIds(const beresta::Element& element, std::set<std::string>& ids);
    void indexPaletteTree(OverlayContext& context, beresta::Element& node);
    std::unique_ptr<beresta::Element> nativePaletteInstance(OverlayContext& context, const beresta::Element& prototype,
        const beresta::Element& op);
    void applyPlacement(OverlayContext& context, const beresta::Element& operation);
    void applyToken(OverlayContext& context, const beresta::Element& operation);
    void materializeTokens(OverlayContext& context);
    void checkOverlayDesign(UiDocument& document);
    struct DesignBackdrop
    {
        Color dark{1, 1, 1, 1};
        Color light{1, 1, 1, 1};
        double minimum = 1;
        double maximum = 1;
    };
    void designBackdrop(const style::Paint& fill, float opacity, DesignBackdrop& backdrop);
    double designContrast(Color text, float opacity, const DesignBackdrop& backdrop);
    using TargetTransforms = std::map<const ui::Node*, std::vector<Affine2D>>;
    void recordTargetTransform(const ui::Node& node, TargetTransforms& transforms);
    void checkTargetSizes(ui::Node& root, float minimum, const TargetTransforms& transforms);
    void inspectDesignStates(UiDocument& document, const std::function<void(ui::Node&, bool)>& inspect);
    void replaceScopedProperties(beresta::Element& element, const TokenMap& tokens);
    void applyRebind(OverlayContext& context, const beresta::Element& operation);
    void applyShortcut(OverlayContext& context, const beresta::Element& operation);
    std::string addSequence(const OverlayContext& context, const beresta::Element& op, const std::string& label);
    const beresta::Element* overlayResource(const OverlayContext& context, std::string_view group,
        std::string_view tag, std::string_view key, std::string_view value);
    std::unique_ptr<beresta::Element> paletteInstance(OverlayContext& context, const beresta::Element& op);
    void denyOverlay(const beresta::Element& element, std::string_view reason);
    beresta::Document parseOverlay(std::string_view xml);
}

#endif
