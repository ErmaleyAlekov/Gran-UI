#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/style/value_parsers.hpp"
#include "gran/animation/keyframes.hpp"
#include <cmath>
#include <limits>

namespace
{
    // Тесты для parseAnimations
    void testParseAnimations()
    {
        auto animations = gran::style::parseAnimations("pulse 1.2s ease-in-out 0.2s infinite alternate both");
        TEST_ASSERT(animations.size() == 1);
        TEST_ASSERT(animations[0].name == "pulse");
        TEST_ASSERT_NEAR(animations[0].duration, 1.2f, 0.001f);
        TEST_ASSERT_NEAR(animations[0].delay, 0.2f, 0.001f);
        TEST_ASSERT(animations[0].easing.kind == gran::style::EasingKind::CUBIC_BEZIER);
        TEST_ASSERT(std::isinf(animations[0].iterations));
        TEST_ASSERT(animations[0].direction == gran::style::AnimationDirection::ALTERNATE);
        TEST_ASSERT(animations[0].fill == gran::style::AnimationFill::BOTH);

        animations = gran::style::parseAnimations("spin 500ms linear 3 reverse, fade 1s");
        TEST_ASSERT(animations.size() == 2);
        TEST_ASSERT(animations[0].name == "spin");
        TEST_ASSERT_NEAR(animations[0].duration, 0.5f, 0.001f);
        TEST_ASSERT(animations[0].easing.kind == gran::style::EasingKind::LINEAR);
        TEST_ASSERT(animations[0].iterations == 3);
        TEST_ASSERT(animations[0].direction == gran::style::AnimationDirection::REVERSE);
        TEST_ASSERT(animations[0].fill == gran::style::AnimationFill::NONE);

        TEST_ASSERT(animations[1].name == "fade");
        TEST_ASSERT_NEAR(animations[1].duration, 1.0f, 0.001f);
        TEST_ASSERT(animations[1].iterations == 1);
        TEST_ASSERT(animations[1].direction == gran::style::AnimationDirection::NORMAL);

        animations = gran::style::parseAnimations("none");
        TEST_ASSERT(animations.empty());

        TEST_ASSERT_THROWS(gran::style::parseAnimations("pulse"), gran::GranException);
        TEST_ASSERT_THROWS(gran::style::parseAnimations("pulse 1s 2s 3s"), gran::GranException);
        TEST_ASSERT_THROWS(gran::style::parseAnimations("1s pulse"), gran::GranException);
        TEST_ASSERT_THROWS(gran::style::parseAnimations("pulse 1s -1"), gran::GranException);
    }

    // Тесты для animationTiming
    void testAnimationTiming()
    {
        gran::style::AnimationSpec spec;
        spec.duration = 2.0f;
        spec.delay = 1.0f;
        spec.fill = gran::style::AnimationFill::NONE;

        gran::animation::AnimationTiming timing = gran::animation::animationTiming(spec, 0.5);
        TEST_ASSERT(!timing.active);
        TEST_ASSERT(!timing.finished);

        timing = gran::animation::animationTiming(spec, 2.0);
        TEST_ASSERT(timing.active);
        TEST_ASSERT_NEAR(timing.progress, 0.5f, 0.001f);

        timing = gran::animation::animationTiming(spec, 3.5);
        TEST_ASSERT(timing.finished);
        TEST_ASSERT(!timing.active);

        spec.fill = gran::style::AnimationFill::BOTH;
        timing = gran::animation::animationTiming(spec, 0.5);
        TEST_ASSERT(timing.active);
        TEST_ASSERT_NEAR(timing.progress, 0.0f, 0.001f);

        timing = gran::animation::animationTiming(spec, 3.5);
        TEST_ASSERT(timing.active);
        TEST_ASSERT_NEAR(timing.progress, 1.0f, 0.001f);

        spec.iterations = 2.0f;
        spec.direction = gran::style::AnimationDirection::ALTERNATE;
        spec.duration = 1.0f;
        spec.delay = 0.0f;

        timing = gran::animation::animationTiming(spec, 0.25);
        TEST_ASSERT_NEAR(timing.progress, 0.25f, 0.001f);

        timing = gran::animation::animationTiming(spec, 1.25);
        TEST_ASSERT_NEAR(timing.progress, 0.75f, 0.001f);

        timing = gran::animation::animationTiming(spec, 5.0);
        TEST_ASSERT(timing.finished);
        TEST_ASSERT_NEAR(timing.progress, 0.0f, 0.001f);

        spec.iterations = 1.5f;
        spec.fill = gran::style::AnimationFill::FORWARDS;
        spec.duration = 1.0f;

        timing = gran::animation::animationTiming(spec, 10.0);
        TEST_ASSERT_NEAR(timing.progress, 0.5f, 0.001f);

        spec.iterations = std::numeric_limits<float>::infinity();
        spec.duration = 1.0f;

        timing = gran::animation::animationTiming(spec, 1000.25);
        TEST_ASSERT(!timing.finished);
        TEST_ASSERT_NEAR(timing.progress, 0.25f, 0.001f);

        spec.direction = gran::style::AnimationDirection::REVERSE;
        spec.duration = 1.0f;

        timing = gran::animation::animationTiming(spec, 0.25);
        TEST_ASSERT_NEAR(timing.progress, 0.75f, 0.001f);
    }

    // Тесты для buildFrameTrack и sampleTrack
    void testBuildFrameTrackAndSampleTrack()
    {
        gran::style::KeyframesRule rule;
        gran::style::Keyframe k;
        k.offset = 0.5f;
        k.values.push_back({gran::style::PropertyId::OPACITY, gran::style::PropertyValue(0.2f)});
        rule.frames.push_back(k);

        gran::style::ComputedStyle base;
        base.opacity = 1.0f;

        gran::style::Easing linear;
        linear.kind = gran::style::EasingKind::LINEAR;

        gran::animation::FrameTrack track = gran::animation::buildFrameTrack(rule, base, linear);
        TEST_ASSERT(track.mask == (1u << static_cast<unsigned>(gran::animation::Animated::OPACITY)));

        gran::animation::VisualState v = track.base;
        gran::animation::sampleTrack(track, gran::animation::Animated::OPACITY, 0.0f, v);
        TEST_ASSERT_NEAR(v.opacity, 1.0f, 0.001f);

        gran::animation::sampleTrack(track, gran::animation::Animated::OPACITY, 0.25f, v);
        TEST_ASSERT_NEAR(v.opacity, 0.6f, 0.001f);

        gran::animation::sampleTrack(track, gran::animation::Animated::OPACITY, 0.5f, v);
        TEST_ASSERT_NEAR(v.opacity, 0.2f, 0.001f);

        gran::animation::sampleTrack(track, gran::animation::Animated::OPACITY, 0.75f, v);
        TEST_ASSERT_NEAR(v.opacity, 0.6f, 0.001f);

        gran::animation::sampleTrack(track, gran::animation::Animated::OPACITY, 1.0f, v);
        TEST_ASSERT_NEAR(v.opacity, 1.0f, 0.001f);

        // Второй набор: кадры 0% и 100%, у первого своя кривая steps(2) / The second set: 0% and 100% frames, the first one has its own steps(2) curve
        rule.frames.clear();
        gran::style::Keyframe start;
        start.offset = 0.0f;
        start.values.push_back({gran::style::PropertyId::OPACITY, gran::style::PropertyValue(0.0f)});
        start.hasEasing = true;
        start.easing.kind = gran::style::EasingKind::STEPS;
        start.easing.steps = 2;
        start.easing.jumpStart = false;
        rule.frames.push_back(start);
        gran::style::Keyframe end;
        end.offset = 1.0f;
        end.values.push_back({gran::style::PropertyId::OPACITY, gran::style::PropertyValue(1.0f)});
        rule.frames.push_back(end);

        track = gran::animation::buildFrameTrack(rule, base, linear);

        gran::animation::sampleTrack(track, gran::animation::Animated::OPACITY, 0.4f, v);
        TEST_ASSERT_NEAR(v.opacity, 0.0f, 0.001f);

        gran::animation::sampleTrack(track, gran::animation::Animated::OPACITY, 0.6f, v);
        TEST_ASSERT_NEAR(v.opacity, 0.5f, 0.001f);
    }
}

namespace suites
{
    // Регистрация набора / The suite registration
    void registerKeyframeTests(test::TestRunner& runner)
    {
        runner.add("animation: parseAnimations", testParseAnimations);
        runner.add("animation: animationTiming", testAnimationTiming);
        runner.add("animation: buildFrameTrackAndSampleTrack", testBuildFrameTrackAndSampleTrack);
    }
}
