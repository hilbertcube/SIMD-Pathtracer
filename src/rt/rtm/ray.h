#pragma once

#include "vector.h"

namespace rt
{

// Ray is a quantity with direction, P(t) = A + t * b
// Here, A is the ray origin and b is the direction vector

class ray
{
public:
    ray() {}

    ray(const point3f& origin, const vec3f& direction, float time)
        : origin_(origin), direction_(direction), time_(time)
    {
    }

    ray(const point3f& origin, const vec3f& direction) : ray(origin, direction, 0) {}

    [[nodiscard]]
    const point3f& origin() const
    {
        return origin_;
    }

    [[nodiscard]]
    const point3f& direction() const
    {
        return direction_;
    }

    [[nodiscard]]
    point3f at(float t) const
    {
        return origin_ + t * direction_;
    }

    [[nodiscard]]
    float time() const
    {
        return time_;
    }

private:
    point3f origin_;
    vec3f direction_;
    float time_;
};

} // namespace rt
