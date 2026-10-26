#include "gran/layout/internal/layout_internal.hpp"

#include <algorithm>
#include <cmath>

namespace gran::layout::internal
{
    namespace
    {
        // Доля fr гибкой дорожки (0 у негибкой) / The fr fraction of a flexible track (0 for a non-flexible one)
        float fractionOf(const std::vector<style::GridTrack>& tracks, std::size_t index) noexcept
        {
            style::GridTrack track = trackAt(tracks, index);

            if (track.max.kind != style::TrackBoundKind::FRACTION)
                return 0.0f;
            return track.max.value;
        }

        // Одна доля при конечном месте: остаток после негибких дорожек / One fraction with a finite space: the remainder after the rigid tracks
        float spaceUnit(const std::vector<float>& sizes, const std::vector<style::GridTrack>& tracks,
            const std::vector<bool>& flexible, float space) noexcept
        {
            float leftover = space;
            float fractions = 0.0f;

            for (std::size_t index = 0; index < sizes.size(); ++index)
            {
                if (flexible[index])
                    fractions += fractionOf(tracks, index);
                else
                    leftover -= sizes[index];
            }
            if (fractions <= 0.0f)
                return 0.0f;
            // Сумма долей меньше 1 занимает лишь часть остатка, как в CSS / A fraction sum below 1 takes only a part of the remainder, as in CSS
            return std::max(0.0f, leftover / std::max(1.0f, fractions));
        }

        // Одна доля при измерении по содержимому: наибольшее «содержимое / fr» / One fraction when measuring by content: the largest "content / fr"
        float contentUnit(const std::vector<style::GridTrack>& tracks, const std::vector<float>& content) noexcept
        {
            float unit = 0.0f;

            for (std::size_t index = 0; index < content.size(); ++index)
            {
                float fraction = fractionOf(tracks, index);
                if (fraction > 0.0f)
                    unit = std::max(unit, content[index] / fraction);
            }
            return unit;
        }

        // Дорожки, которым доля меньше минимума, становятся негибкими; true — такие были
        // Tracks whose share is below the minimum become rigid; true — there were such tracks
        bool fixTooSmall(const std::vector<float>& sizes, const std::vector<style::GridTrack>& tracks,
            std::vector<bool>& flexible, float unit) noexcept
        {
            bool fixedAny = false;

            for (std::size_t index = 0; index < sizes.size(); ++index)
            {
                if (flexible[index] && unit * fractionOf(tracks, index) < sizes[index])
                {
                    flexible[index] = false;
                    fixedAny = true;
                }
            }
            return fixedAny;
        }
    }

    // Минимум гибкой дорожки уже лежит в sizes (px или 0) / The flexible track minimum already lies in sizes (px or 0)
    void resolveFractions(std::vector<float>& sizes, const std::vector<style::GridTrack>& tracks,
        const std::vector<float>& content, float space)
    {
        std::vector<bool> flexible(sizes.size(), false);
        float unit = contentUnit(tracks, content);

        for (std::size_t index = 0; index < sizes.size(); ++index)
            flexible[index] = fractionOf(tracks, index) > 0.0f;
        for (std::size_t round = 0; round <= sizes.size() && std::isfinite(space); ++round)
        {
            unit = spaceUnit(sizes, tracks, flexible, space);
            if (!fixTooSmall(sizes, tracks, flexible, unit))
                break;
        }
        for (std::size_t index = 0; index < sizes.size(); ++index)
        {
            if (flexible[index])
                sizes[index] = std::max(sizes[index], unit * fractionOf(tracks, index));
        }
    }
}
