#include "gran/style/value_parsers.hpp"
#include "gran/style/internal/value_support.hpp"
#include "gran/style/value_scanner.hpp"

namespace gran::style
{
    using internal::equalsIgnoreCase;
    using internal::splitNumberUnit;
    using internal::throwValueError;

    namespace
    {
        // Граница дорожки: "auto", "48", "48px", "2fr" (доля только при allowFraction)
        // A track bound: "auto", "48", "48px", "2fr" (a fraction only with allowFraction)
        TrackBound parseBound(std::string_view text, bool allowFraction, std::string_view original)
        {
            TrackBound bound;
            std::string_view unit;

            if (equalsIgnoreCase(trimSpaces(text), "auto"))
                return bound;
            if (!splitNumberUnit(trimSpaces(text), bound.value, unit) || bound.value < 0.0f)
                throwValueError(i18n::Message::STYLE_BAD_TRACK, original);
            if (unit.empty() || equalsIgnoreCase(unit, "px"))
                bound.kind = TrackBoundKind::PIXELS;
            else if (allowFraction && equalsIgnoreCase(unit, "fr"))
                bound.kind = TrackBoundKind::FRACTION;
            else
                throwValueError(i18n::Message::STYLE_BAD_TRACK, original);
            return bound;
        }

        /*
        ** Одна дорожка. "1fr" = minmax(auto, 1fr), остальные — minmax(v, v);
        ** у minmax(min, max) минимум не может быть долей.
        **
        ** One track. "1fr" = minmax(auto, 1fr), the others are minmax(v, v);
        ** in minmax(min, max) the minimum cannot be a fraction.
        */
        GridTrack parseTrack(std::string_view text)
        {
            ValueScanner scanner(text);
            std::string_view name = scanner.readIdentifier();
            std::vector<std::string_view> arguments;
            GridTrack track;

            if (equalsIgnoreCase(name, "minmax"))
            {
                if (!scanner.readArguments(arguments) || !scanner.atEnd() || arguments.size() != 2)
                    throwValueError(i18n::Message::STYLE_BAD_TRACK, text);
                track.min = parseBound(arguments[0], false, text);
                track.max = parseBound(arguments[1], true, text);
                return track;
            }
            track.max = parseBound(text, true, text);
            track.min = track.max;
            if (track.max.kind == TrackBoundKind::FRACTION)
                track.min = TrackBound{TrackBoundKind::AUTO, 0.0f};
            return track;
        }
    }

    // Дорожки через запятые или пробелы (скобки minmax не делятся)
    // Tracks separated by commas or spaces (minmax parentheses are not split)
    std::vector<GridTrack> parseTracks(std::string_view text)
    {
        std::vector<GridTrack> tracks;

        for (std::string_view part : splitTopLevel(text, ','))
        {
            for (std::string_view token : splitSpaces(part))
                tracks.push_back(parseTrack(token));
        }
        if (tracks.empty())
            throwValueError(i18n::Message::STYLE_BAD_TRACK, text);
        return tracks;
    }
}
