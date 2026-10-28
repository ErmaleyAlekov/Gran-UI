#include "gran/animation/visual_state.hpp"

#include <algorithm>
#include <cmath>

namespace gran::animation
{
    namespace
    {
        // Координата кривой Безье с концами 0 и 1 и контрольными a, b / A Bezier curve coordinate with ends 0 and 1 and controls a, b
        float bezier(float t, float a, float b) noexcept
        {
            float u = 1.0f - t;
            return 3.0f * u * u * t * a + 3.0f * u * t * t * b + t * t * t;
        }

        // Производная координаты по t / The coordinate derivative by t
        float bezierSlope(float t, float a, float b) noexcept
        {
            float u = 1.0f - t;
            return 3.0f * u * u * a + 6.0f * u * t * (b - a) + 3.0f * t * t * (1.0f - b);
        }

        /*
        ** Параметр t, при котором x(t) = x: сначала метод Ньютона (быстро сходится почти всегда),
        ** при пологом участке — деление пополам (x монотонна, так как x1, x2 ∈ [0, 1]).
        **
        ** The parameter t at which x(t) = x: first Newton's method (converges fast almost always),
        ** on a flat part — bisection (x is monotonic since x1, x2 ∈ [0, 1]).
        */
        float solveX(float x, float x1, float x2) noexcept
        {
            float t = x;

            for (int iteration = 0; iteration < 8; ++iteration)
            {
                float error = bezier(t, x1, x2) - x;
                float slope = bezierSlope(t, x1, x2);
                if (std::fabs(error) < 1e-5f)
                    return t;
                if (std::fabs(slope) < 1e-6f)
                    break;
                t = std::min(1.0f, std::max(0.0f, t - error / slope));
            }
            float low = 0.0f;
            float high = 1.0f;
            for (int iteration = 0; iteration < 32 && high - low > 1e-6f; ++iteration)
            {
                t = (low + high) * 0.5f;
                if (bezier(t, x1, x2) < x)
                    low = t;
                else
                    high = t;
            }
            return t;
        }
    }

    // linear — тождество, steps — ступени (jump-start поднимает первую), cubic-bezier — по кривой
    // linear is the identity, steps are stairs (jump-start raises the first), cubic-bezier follows the curve
    float evaluateEasing(const style::Easing& easing, float t) noexcept
    {
        float clamped = std::min(1.0f, std::max(0.0f, t));

        if (easing.kind == style::EasingKind::LINEAR)
            return clamped;
        if (easing.kind == style::EasingKind::STEPS)
        {
            float steps = static_cast<float>(std::max(1, easing.steps));
            float step = std::floor(clamped * steps);
            if (easing.jumpStart)
                step += 1.0f;
            return std::min(1.0f, step / steps);
        }
        if (clamped <= 0.0f || clamped >= 1.0f)
            return clamped;
        return bezier(solveX(clamped, easing.x1, easing.x2), easing.y1, easing.y2);
    }
}
