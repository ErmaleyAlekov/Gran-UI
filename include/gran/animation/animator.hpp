#ifndef GRAN_ANIMATION_ANIMATOR_HPP
#define GRAN_ANIMATION_ANIMATOR_HPP

#include "gran/animation/keyframes.hpp"
#include "gran/ui/style_pass.hpp"

#include <cstdint>
#include <vector>

namespace gran::animation
{
    // Ход одного свойства: начало, задержка, длительность, кривая / The course of one property: the start, the delay, the duration, the curve
    struct Track
    {
        double start = 0.0;
        float delay = 0.0f;
        float duration = 0.0f;
        style::Easing easing;
    };

    // Анимация узла: откуда, куда и ходы свойств (бит маски — свойство идёт) / A node animation: from, to and the property tracks (a mask bit — the property runs)
    struct NodeAnimation
    {
        ui::Node* node = nullptr;
        VisualState from;
        VisualState to;
        Track tracks[ANIMATED_COUNT];
        std::uint32_t mask = 0;
    };

    // Анимация по ключевым кадрам на узле: описание, время запуска, кадры, закончилась ли
    // A keyframe animation on a node: the description, the start time, the frames, whether it has finished
    struct KeyframeRun
    {
        ui::Node* node = nullptr;
        style::AnimationSpec spec;
        double start = 0.0;
        FrameTrack track;
        bool finished = false;
    };

    /*
    ** Переходы CSS (transition). При пересчёте стиля узла (смена состояния hover/pressed...)
    ** изменившиеся анимируемые свойства с описанным переходом не прыгают к новому значению,
    ** а плавно идут от текущего показанного. Пересчёт посреди анимации начинает новую
    ** от текущей точки — без рывка (как в браузерах).
    **
    ** CSS transitions. When a node style is recomputed (a hover/pressed... state change)
    ** the changed animatable properties with a described transition do not jump to the new value,
    ** they move smoothly from the currently shown one. A recompute in the middle of an animation starts
    ** a new one from the current point — without a jerk (as in browsers).
    **
    ** Анимации по ключевым кадрам (свойство animation + <Keyframes>) запускаются, когда имя
    ** появляется в стиле узла (в том числе при загрузке), и снимаются, когда исчезает. Закончившаяся
    ** анимация не повторяется, пока имя остаётся в стиле (как в CSS). Свойства, которые ведёт
    ** анимация по кадрам, переходами не анимируются: её значения сильнее.
    **
    ** Keyframe animations (the animation property + <Keyframes>) start when the name
    ** appears in the node style (on load too) and are dropped when it disappears. A finished
    ** animation does not repeat while the name stays in the style (as in CSS). The properties driven
    ** by a keyframe animation are not animated by transitions: its values are stronger.
    */
    class Animator : public ui::StyleObserver
    {
    public:
        // ---- animator.cpp ----

        // Время текущего кадра (секунды) / The current frame time (seconds)
        void setTime(double now) noexcept;

        // Начать переходы для изменившихся свойств / Start transitions for the changed properties
        void styleChanged(ui::Node& node, const style::ComputedStyle& shown, style::ComputedStyle& next) override;

        // Шаг анимаций; true — ещё идут (нужен следующий кадр) / An animation step; true — still running (the next frame is needed)
        bool tick();

        // Узел удаляется из дерева / The node is removed from the tree
        void forget(const ui::Node& node);

        // ---- animator_keyframes.cpp ----

        // Наборы ключевых кадров документа (nullptr — анимаций по кадрам нет); живут дольше аниматора / The document keyframe sets (nullptr — no keyframe animations); they outlive the animator
        void setStyleSheet(const style::StyleSheet* sheet) noexcept;

        // Первый расчёт стиля узла: запуск его анимаций по кадрам / The first node style computation: starting its keyframe animations
        void styleCreated(ui::Node& node, style::ComputedStyle& next) override;

    private:
        // Запуски и снятия анимаций по кадрам узла, их значения — в next; результат — ведомые свойства (биты Animated)
        // Starting and dropping the node keyframe animations, their values go into next; the result is the driven properties (Animated bits)
        std::uint32_t syncKeyframes(ui::Node& node, style::ComputedStyle& next);

        // ---- animator_tick.cpp ----

        // Шаг анимаций по кадрам; true — какая-то ещё идёт на видимом узле / A keyframe animation step; true — some still runs on a visible node
        bool tickKeyframes();

        std::vector<NodeAnimation> _animations;
        std::vector<KeyframeRun> _runs;
        const style::StyleSheet* _sheet = nullptr;
        double _now = 0.0;
    };
}

#endif
