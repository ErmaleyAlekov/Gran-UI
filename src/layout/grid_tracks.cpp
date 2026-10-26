#include "gran/layout/internal/layout_internal.hpp"

#include <algorithm>
#include <cmath>

namespace gran::layout::internal
{
    namespace
    {
        using style::TrackBoundKind;

        /*
        ** Негибкая дорожка: минимум (px или 0) и максимум (px или содержимое).
        ** "48" → 48; "auto" → содержимое; minmax(100, auto) → max(100, содержимое);
        ** minmax(auto, 200) → min(содержимое, 200). У fr здесь только минимум.
        **
        ** A non-flexible track: the minimum (px or 0) and the maximum (px or the content).
        ** "48" → 48; "auto" → the content; minmax(100, auto) → max(100, the content);
        ** minmax(auto, 200) → min(the content, 200). For fr only the minimum is taken here.
        */
        float fixedPart(const style::GridTrack& track, float content) noexcept
        {
            float minimum = 0.0f;

            if (track.min.kind == TrackBoundKind::PIXELS)
                minimum = track.min.value;
            if (track.max.kind == TrackBoundKind::FRACTION)
                return minimum;
            if (track.max.kind == TrackBoundKind::PIXELS)
                return std::max(minimum, std::min(content, track.max.value));
            return std::max(minimum, content);
        }

        // Избыток охватывающего элемента делится поровну между его auto-дорожками
        // The excess of a spanning item is shared equally between its auto tracks
        void distributeSpanning(std::vector<float>& sizes, const std::vector<style::GridTrack>& tracks,
            const std::vector<GridContribution>& contributions, float gap)
        {
            for (const GridContribution& contribution : contributions)
            {
                float excess = contribution.size - spanSize(sizes, contribution.first, contribution.span, gap);
                std::vector<std::size_t> autos;
                for (std::size_t index = contribution.first; index < contribution.first + contribution.span; ++index)
                {
                    if (trackAt(tracks, index).max.kind == TrackBoundKind::AUTO)
                        autos.push_back(index);
                }
                if (contribution.span < 2 || excess <= 0.0f || autos.empty())
                    continue;
                for (std::size_t index : autos)
                    sizes[index] += excess / static_cast<float>(autos.size());
            }
        }
    }

    // Размеры дорожек по шагам: вклады с охватом 1, негибкие, охватывающие, доли
    // The track sizes step by step: the span-1 contributions, the non-flexible ones, the spanning ones, the fractions
    std::vector<float> sizeTracks(const std::vector<style::GridTrack>& tracks,
        const std::vector<GridContribution>& contributions, float available, float gap)
    {
        std::size_t count = trackCount(tracks);
        std::vector<float> content(count, 0.0f);
        std::vector<float> sizes(count, 0.0f);

        for (const GridContribution& contribution : contributions)
        {
            if (contribution.span == 1)
                content[contribution.first] = std::max(content[contribution.first], contribution.size);
        }
        for (std::size_t index = 0; index < count; ++index)
            sizes[index] = fixedPart(trackAt(tracks, index), content[index]);
        distributeSpanning(sizes, tracks, contributions, gap);
        resolveFractions(sizes, tracks, content, available - gap * static_cast<float>(count - 1));
        return sizes;
    }
}
