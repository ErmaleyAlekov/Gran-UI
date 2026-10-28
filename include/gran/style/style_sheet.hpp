#ifndef GRAN_STYLE_STYLE_SHEET_HPP
#define GRAN_STYLE_STYLE_SHEET_HPP

#include "gran/style/computed_style.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace gran::style
{
    /*
    ** Состояния элемента. Порядок — приоритет: более позднее состояние перекрывает
    ** более раннее (нажатие важнее наведения, отключение важнее всего, кроме отметки).
    **
    ** The element states. The order is the priority: a later state overrides
    ** an earlier one (pressing beats hovering, disabling beats everything except checking).
    */
    enum class State : std::uint8_t { HOVER, PRESSED, FOCUSED, DISABLED, CHECKED };

    // Число состояний / The state count
    constexpr std::size_t STATE_COUNT = 5;

    // Бит состояния в маске (cascade.cpp) / The state bit in a mask (cascade.cpp)
    std::uint8_t stateBit(State state) noexcept;

    // Объявления одного источника: основные и по состояниям / The declarations of one source: base and per state
    struct DeclarationBlock
    {
        PropertySet base;
        PropertySet states[STATE_COUNT];
    };

    // Именованный стиль <Style>; basedOn — родительский стиль (без владения)
    // A named <Style>; basedOn is the parent style (not owned)
    struct StyleRule
    {
        std::string name;
        DeclarationBlock block;
        const StyleRule* basedOn = nullptr;
    };

    /*
    ** Ключевой кадр <Frame at="50%" …>: место на шкале анимации (0..1), анимируемые свойства
    ** и кривая участка до следующего кадра (hasEasing == false — кривая самой анимации).
    **
    ** A <Frame at="50%" …> keyframe: the place on the animation scale (0..1), the animatable properties
    ** and the curve of the segment up to the next frame (hasEasing == false — the curve of the animation itself).
    */
    struct Keyframe
    {
        float offset = 0.0f;
        PropertySet values;
        bool hasEasing = false;
        Easing easing;
    };

    // Набор ключевых кадров <Keyframes name>; кадры — по возрастанию места / A <Keyframes name> set; the frames are in ascending place order
    struct KeyframesRule
    {
        std::string name;
        std::vector<Keyframe> frames;
    };

    /*
    ** Набор стилей окна. Владеет правилами; указатели на них стабильны
    ** (unique_ptr), поэтому узлы могут хранить ссылку на правило.
    **
    ** The window style set. It owns the rules; the pointers to them are stable
    ** (unique_ptr), so nodes can keep a reference to a rule.
    */
    class StyleSheet
    {
    public:
        // Добавляет правило и возвращает его / Adds a rule and returns it
        StyleRule& add(std::string name);

        // Правило по имени или nullptr / A rule by name or nullptr
        StyleRule* find(std::string_view name) const noexcept;

        // Имена всех правил в порядке объявления (для проверки фрагментов разметки) / The names of all rules in the declaration order (for checking markup fragments)
        std::vector<std::string_view> names() const;

        // Добавляет набор ключевых кадров; ссылка живёт до следующего добавления / Adds a keyframe set; the reference lives until the next addition
        KeyframesRule& addKeyframes(std::string name);

        // Набор ключевых кадров по имени или nullptr / A keyframe set by name or nullptr
        const KeyframesRule* findKeyframes(std::string_view name) const noexcept;

    private:
        std::vector<std::unique_ptr<StyleRule>> _rules;
        std::vector<KeyframesRule> _keyframes;
    };

    /*
    ** Источники объявлений узла для каскада (без владения). defaults — встроенный стиль
    ** вида элемента (как стили браузера по умолчанию для <button>), он слабее всех.
    **
    ** The node declaration sources for the cascade (not owned). defaults is the built-in style
    ** of the element kind (like the browser default styles for <button>), it is the weakest.
    */
    struct CascadeInput
    {
        const ComputedStyle* parent = nullptr;
        const StyleRule* defaults = nullptr;
        const StyleRule* rule = nullptr;
        const DeclarationBlock* inlineBlock = nullptr;
        std::uint8_t stateMask = 0;
    };

    // ---- cascade.cpp ----

    /*
    ** Каскад: умолчания ← наследуемые свойства родителя ← стиль вида элемента ← цепочка basedOn
    ** (от дальнего) ← атрибуты элемента ← по каждому активному состоянию: стиль вида, стиль, атрибуты.
    **
    ** The cascade: defaults ← the inherited parent properties ← the element kind style ← the basedOn chain
    ** (from the farthest) ← the element attributes ← for each active state: the kind style, the style, the attributes.
    */
    ComputedStyle computeStyle(const CascadeInput& input);

    // ---- apply_declaration.cpp ----

    // Записывает одно объявление в вычисленный стиль / Writes one declaration into the computed style
    void applyDeclaration(ComputedStyle& style, const Declaration& declaration);

    // Копирует наследуемые свойства (текст) родителя / Copies the inherited (text) properties of the parent
    void inheritFrom(ComputedStyle& style, const ComputedStyle& parent);
}

#endif
