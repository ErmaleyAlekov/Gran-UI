#include "gran/markup/internal/build_context.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/style/value_parsers.hpp"

#include "beresta.hpp"

#include <algorithm>

namespace gran::markup::internal
{
    namespace
    {
        // Порядок кадров по месту (для устойчивой сортировки) / The frame order by place (for a stable sort)
        bool earlier(const style::Keyframe& first, const style::Keyframe& second) noexcept
        {
            return first.offset < second.offset;
        }

        /*
        ** Один кадр: место "50%" (вид и предел 100% проверила схема) → 0.5, своя кривая
        ** участка и анимируемые свойства (через тот же разбор, что и у стилей: тема, ошибки с позицией).
        **
        ** One frame: the place "50%" (the schema has checked the form and the 100% limit) → 0.5, its own segment
        ** curve and the animatable properties (through the same parsing as styles: the theme, errors with a position).
        */
        style::Keyframe readFrame(const beresta::Element& frame, BuildContext& context)
        {
            style::Keyframe keyframe;
            const std::string& at = *findAttribute(frame, "at");
            const std::string* easing = findAttribute(frame, "easing");

            keyframe.offset = style::parseNumber(std::string_view(at).substr(0, at.size() - 1)) / 100.0f;
            if (easing != nullptr)
            {
                try
                {
                    keyframe.easing = style::parseEasing(*easing);
                }
                catch (const GranException& error)
                {
                    throwAttributeError(frame, "easing", error.getMessage());
                }
                keyframe.hasEasing = true;
            }
            readDeclarations(frame, keyframe.values, context);
            return keyframe;
        }
    }

    // Кадры могут идти в разметке в любом порядке; равные места сохраняют порядок разметки
    // The frames may go in any order in the markup; equal places keep the markup order
    void readKeyframes(const beresta::Element& keyframes, BuildContext& context)
    {
        std::vector<style::Keyframe> frames;

        for (const beresta::Element* frame = firstElementChild(keyframes); frame != nullptr;
             frame = nextElementSibling(*frame))
            frames.push_back(readFrame(*frame, context));
        std::stable_sort(frames.begin(), frames.end(), earlier);
        context.document.styles.addKeyframes(*findAttribute(keyframes, "name")).frames = std::move(frames);
    }

    /*
    ** Строгая проверка вместо молчаливого пропуска (как сделал бы браузер): опечатка в имени
    ** видна сразу при загрузке. <Keyframes> в схеме идут раньше стилей и элементов, поэтому
    ** к моменту чтения animation все наборы уже известны.
    **
    ** A strict check instead of a silent skip (as a browser would do): a typo in the name
    ** is visible at once on loading. <Keyframes> go before the styles and the elements in the schema, so
    ** all the sets are known by the time animation is read.
    */
    void checkAnimationNames(const style::PropertyValue& value, const BuildContext& context)
    {
        const std::vector<style::AnimationSpec>* specs = std::get_if<std::vector<style::AnimationSpec>>(&value);

        if (specs == nullptr)
            return;
        for (const style::AnimationSpec& spec : *specs)
        {
            if (context.document.styles.findKeyframes(spec.name) == nullptr)
                throw GranException(ErrorCode::RESOURCE,
                    i18n::format(i18n::Message::RESOURCE_UNKNOWN_KEYFRAMES, {spec.name}));
        }
    }
}
