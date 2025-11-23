#pragma once

#include <cmath>
#include "vector.h"

namespace rt {

/// @brief Orthonormal basis built around a single direction (used as the local w axis).
class ONB
{
public:
    ONB() = default;

    explicit ONB(const vec3f& n)
    {
        axis[2] = unit_vector(n);
        vec3f a = (std::fabs(axis[2].x()) > 0.9f) ? vec3f(0, 1, 0) : vec3f(1, 0, 0);
        axis[1] = unit_vector(cross(axis[2], a));
        axis[0] = cross(axis[2], axis[1]);
    }

    const vec3f& u() const { return axis[0]; }
    const vec3f& v() const { return axis[1]; }
    const vec3f& w() const { return axis[2]; }

    /// @brief Transform from basis coordinates to world space.
    vec3f transform(const vec3f& v) const
    {
        return (v[0] * axis[0]) + (v[1] * axis[1]) + (v[2] * axis[2]);
    }

private:
    vec3f axis[3];
};

} // namespace rt
