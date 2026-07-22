#pragma once
#include <cassert>

#include "rt/math/constants.h"

namespace rt
{

// an interval class to manage real-valued intervals with a minimum and a maximum
class interval
{
public:
    float min, max;

    interval() : min(+INF), max(-INF) {}

    interval(float min, float max) : min(min), max(max) {}

    interval(const interval& first, const interval& second)
    {
        // Create the interval tightly enclosing the two input intervals.
        min = first.min <= second.min ? first.min : second.min;
        max = first.max >= second.max ? first.max : second.max;
    }

    float size() const { return max - min; }

    bool contains(float value) const { return min <= value && value <= max; }

    bool surrounds(float value) const { return min < value && value < max; }

    float clamp(float value) const
    {
        if (value < min)
        {
            return min;
        }
        if (value > max)
        {
            return max;
        }
        return value;
    }

    interval expand(float delta) const
    {
        auto padding = delta / 2;
        return interval(min - padding, max + padding);
    }

    static const interval empty, universe;
};

inline const interval interval::empty    = interval(+INF, -INF);
inline const interval interval::universe = interval(-INF, +INF);

inline interval operator+(const interval& range, float displacement)
{
    return interval(range.min + displacement, range.max + displacement);
}

inline interval operator+(float displacement, const interval& range)
{
    return range + displacement;
}

} // namespace rt
