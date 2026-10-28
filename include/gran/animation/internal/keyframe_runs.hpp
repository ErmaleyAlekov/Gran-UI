#ifndef GRAN_ANIMATION_INTERNAL_KEYFRAME_RUNS_HPP
#define GRAN_ANIMATION_INTERNAL_KEYFRAME_RUNS_HPP

#include "gran/animation/animator.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace gran::animation::internal
{
    // ---- keyframe_runs.cpp ----

    // Снять анимации узла, чьих имён больше нет в specs / Drop the node animations whose names are no longer in specs
    void dropStaleRuns(std::vector<KeyframeRun>& runs, const ui::Node& node, const std::vector<style::AnimationSpec>& specs);

    // Анимация узла по имени или nullptr / The node animation by name or nullptr
    KeyframeRun* findRun(std::vector<KeyframeRun>& runs, const ui::Node& node, const std::string& name) noexcept;

    // Значения анимации в стиль target: выборка кадров (active) или базовые значения / The animation values into the target style: the frame sample (active) or the base values
    void writeRun(const KeyframeRun& run, const AnimationTiming& timing, style::ComputedStyle& target);

    // Наследуемый цвет вниз к потомкам, которые его не переопределили / An inherited color down to the descendants that did not override it
    void inheritDown(ui::Node& node, Animated property, const VisualState& value);

    // Анимированные color/accentColor узла (биты mask) — его потомкам / The animated color/accentColor of the node (the mask bits) to its descendants
    void spreadColors(ui::Node& node, std::uint32_t mask);
}

#endif
