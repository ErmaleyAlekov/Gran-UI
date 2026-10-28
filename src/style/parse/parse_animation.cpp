#include "gran/style/value_parsers.hpp"
#include "gran/style/internal/value_support.hpp"
#include "gran/style/value_scanner.hpp"

#include <limits>

namespace gran::style
{
    using internal::equalsIgnoreCase;
    using internal::throwValueError;

    namespace
    {
        /*
        ** Слова направления и заполнения. "none" — заполнение по умолчанию: внутри описания
        ** анимации оно законно, а всё значение "none" разбирает parseAnimations.
        **
        ** The direction and fill words. "none" is the default fill: inside an animation description
        ** it is legal, and the whole "none" value is handled by parseAnimations.
        */
        bool readKeyword(std::string_view token, AnimationSpec& spec)
        {
            static const struct { const char* word; AnimationDirection direction; } DIRECTIONS[] = {
                {"normal", AnimationDirection::NORMAL}, {"reverse", AnimationDirection::REVERSE},
                {"alternate", AnimationDirection::ALTERNATE}, {"alternate-reverse", AnimationDirection::ALTERNATE_REVERSE}};
            static const struct { const char* word; AnimationFill fill; } FILLS[] = {
                {"none", AnimationFill::NONE}, {"forwards", AnimationFill::FORWARDS},
                {"backwards", AnimationFill::BACKWARDS}, {"both", AnimationFill::BOTH}};

            for (const auto& entry : DIRECTIONS)
            {
                if (equalsIgnoreCase(token, entry.word))
                {
                    spec.direction = entry.direction;
                    return true;
                }
            }
            for (const auto& entry : FILLS)
            {
                if (equalsIgnoreCase(token, entry.word))
                {
                    spec.fill = entry.fill;
                    return true;
                }
            }
            return false;
        }

        /*
        ** Слово после имени: время (первое — длительность, второе — задержка), число повторов,
        ** infinite, направление, заполнение; всё прочее — кривая (её ошибку сообщит parseEasing).
        **
        ** A word after the name: a time (the first is the duration, the second is the delay), a repeat count,
        ** infinite, a direction, a fill; anything else is the curve (parseEasing reports its error).
        */
        void readToken(std::string_view token, int& times, AnimationSpec& spec, std::string_view part)
        {
            bool numeric = (token[0] >= '0' && token[0] <= '9') || token[0] == '.';

            if (numeric && (token.back() == 's' || token.back() == 'S'))
            {
                if (times > 1)
                    throwValueError(i18n::Message::STYLE_BAD_ANIMATION, part);
                if (times == 0)
                    spec.duration = parseDuration(token);
                else
                    spec.delay = parseDuration(token);
                ++times;
            }
            else if (numeric)
                spec.iterations = parseNumber(token);
            else if (equalsIgnoreCase(token, "infinite"))
                spec.iterations = std::numeric_limits<float>::infinity();
            else if (!readKeyword(token, spec))
                spec.easing = parseEasing(token);
        }

        // Одно описание: имя и хотя бы длительность / One description: the name and at least the duration
        AnimationSpec parseOne(std::string_view part)
        {
            std::vector<std::string_view> tokens = splitSpaces(part);
            AnimationSpec spec;
            int times = 0;

            if (tokens.size() < 2 || !((tokens[0][0] >= 'a' && tokens[0][0] <= 'z')
                || (tokens[0][0] >= 'A' && tokens[0][0] <= 'Z')))
                throwValueError(i18n::Message::STYLE_BAD_ANIMATION, part);
            spec.name = std::string(tokens[0]);
            for (std::size_t index = 1; index < tokens.size(); ++index)
                readToken(tokens[index], times, spec, part);
            if (times == 0 || spec.iterations < 0.0f)
                throwValueError(i18n::Message::STYLE_BAD_ANIMATION, part);
            return spec;
        }
    }

    // "none" — без анимаций (так состояние Disabled снимает анимацию стиля) / "none" — no animations (this way the Disabled state removes a style animation)
    std::vector<AnimationSpec> parseAnimations(std::string_view text)
    {
        std::vector<AnimationSpec> animations;

        if (equalsIgnoreCase(trimSpaces(text), "none"))
            return animations;
        for (std::string_view part : splitTopLevel(text, ','))
            animations.push_back(parseOne(trimSpaces(part)));
        return animations;
    }
}
