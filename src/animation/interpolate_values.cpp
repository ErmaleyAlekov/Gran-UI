#include "gran/animation/internal/interpolation.hpp"

namespace gran::animation::internal
{
    namespace
    {
        // Нейтральный шаг того же вида: сдвиг 0, масштаб 1, поворот и наклон 0 / A neutral step of the same kind: shift 0, scale 1, rotation and skew 0
        style::TransformStep neutralStep(const style::TransformStep& step) noexcept
        {
            style::TransformStep neutral{step.kind, 0.0f, 0.0f};

            if (step.kind == style::TransformKind::SCALE)
                neutral = style::TransformStep{step.kind, 1.0f, 1.0f};
            return neutral;
        }

        // Пустое преобразование заменяется нейтральным той же формы, что и другое / An empty transform is replaced with a neutral one of the other's shape
        style::Transform neutralLike(const style::Transform& own, const style::Transform& other)
        {
            style::Transform result = own;

            if (result.empty())
            {
                for (const style::TransformStep& step : other)
                    result.push_back(neutralStep(step));
            }
            return result;
        }
    }

    // Линейная смесь / A linear mix
    float mixNumber(float a, float b, float k) noexcept
    {
        return a + (b - a) * k;
    }

    // Предумножение, смесь, обратное деление на альфу / Premultiplication, mixing, dividing back by the alpha
    Color mixColor(const Color& from, const Color& to, float k) noexcept
    {
        float alpha = mixNumber(from.a, to.a, k);
        Color result{0.0f, 0.0f, 0.0f, alpha};

        if (alpha <= 0.0f)
            return result;
        result.r = mixNumber(from.r * from.a, to.r * to.a, k) / alpha;
        result.g = mixNumber(from.g * from.a, to.g * to.a, k) / alpha;
        result.b = mixNumber(from.b * from.a, to.b * to.a, k) / alpha;
        return result;
    }

    // Шаги попарно при совпадении видов, иначе — значение конца / Steps pairwise when the kinds match, otherwise the end value
    style::Transform mixTransform(const style::Transform& from, const style::Transform& to, float k)
    {
        style::Transform start = neutralLike(from, to);
        style::Transform end = neutralLike(to, from);
        style::Transform result;

        // Несовместимые формы не смешиваются: в самой точке начала — начало, дальше — конец
        // Incompatible shapes are not mixed: at the start point itself — the start, further on — the end
        if (start.size() != end.size() && k <= 0.0f)
            return from;
        if (start.size() != end.size())
            return to;
        for (std::size_t index = 0; index < start.size(); ++index)
        {
            if (start[index].kind != end[index].kind && k <= 0.0f)
                return from;
            if (start[index].kind != end[index].kind)
                return to;
            result.push_back(style::TransformStep{start[index].kind, mixNumber(start[index].x, end[index].x, k),
                mixNumber(start[index].y, end[index].y, k)});
        }
        return result;
    }
}
